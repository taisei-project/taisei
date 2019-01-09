/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "resource.h"
#include "config.h"
#include "video.h"
#include "menu/mainmenu.h"
#include "events.h"
#include "taskmanager.h"

#include "texture.h"
#include "animation.h"
#include "sfx.h"
#include "bgm.h"
#include "shader_object.h"
#include "shader_program.h"
#include "model.h"
#include "postprocess.h"
#include "sprite.h"
#include "font.h"

#include "renderer/common/backend.h"

ResourceHandler *_handlers[] = {
	[RES_TEXTURE] = &texture_res_handler,
	[RES_ANIM] = &animation_res_handler,
	[RES_SFX] = &sfx_res_handler,
	[RES_BGM] = &bgm_res_handler,
	[RES_MODEL] = &model_res_handler,
	[RES_POSTPROCESS] = &postprocess_res_handler,
	[RES_SPRITE] = &sprite_res_handler,
	[RES_FONT] = &font_res_handler,
	[RES_SHADER_OBJECT] = &shader_object_res_handler,
	[RES_SHADER_PROGRAM] = &shader_program_res_handler,
};

typedef enum ResourceStatus {
	RES_STATUS_LOADING,
	RES_STATUS_LOADED,
	RES_STATUS_FAILED,
} ResourceStatus;

typedef struct InternalResource {
	Resource res;
	ResourceStatus status;
	SDL_mutex *mutex;
	SDL_cond *cond;
	Task *async_task;
} InternalResource;

typedef struct ResourceAsyncLoadData {
	InternalResource *ires;
	char *path;
	char *name;
	ResourceFlags flags;
	void *opaque;
} ResourceAsyncLoadData;

static inline ResourceHandler* get_handler(ResourceType type) {
	return *(_handlers + type);
}

static inline ResourceHandler* get_ires_handler(InternalResource *ires) {
	return get_handler(ires->res.type);
}

static void alloc_handler(ResourceHandler *h) {
	assert(h != NULL);
	ht_create(&h->private.mapping);
}

static const char* type_name(ResourceType type) {
	return get_handler(type)->typename;
}

struct valfunc_arg {
	ResourceType type;
};

static void* valfunc_begin_load_resource(void* arg) {
	ResourceType type = ((struct valfunc_arg*)arg)->type;

	InternalResource *ires = calloc(1, sizeof(InternalResource));
	ires->res.type = type;
	ires->status = RES_STATUS_LOADING;
	ires->mutex = SDL_CreateMutex();
	ires->cond = SDL_CreateCond();

	return ires;
}

static bool try_begin_load_resource(ResourceType type, const char *name, InternalResource **out_ires) {
	ResourceHandler *handler = get_handler(type);
	struct valfunc_arg arg = { type };
	return ht_try_set(&handler->private.mapping, name, &arg, valfunc_begin_load_resource, (void**)out_ires);
}

static void load_resource_finish(InternalResource *ires, void *opaque, const char *path, const char *name, char *allocated_path, char *allocated_name, ResourceFlags flags);

static void finish_async_load(InternalResource *ires, ResourceAsyncLoadData *data) {
	assert(ires == data->ires);
	assert(ires->status == RES_STATUS_LOADING);
	load_resource_finish(ires, data->opaque, data->path, data->name, data->path, data->name, data->flags);
	SDL_CondBroadcast(data->ires->cond);
	assert(ires->status != RES_STATUS_LOADING);
	free(data);
}

static ResourceStatus wait_for_resource_load(InternalResource *ires, uint32_t want_flags) {
	SDL_LockMutex(ires->mutex);

	if(ires->async_task != NULL && is_main_thread()) {
		assert(ires->status == RES_STATUS_LOADING);

		ResourceAsyncLoadData *data;
		Task *task = ires->async_task;
		ires->async_task = NULL;

		SDL_UnlockMutex(ires->mutex);

		if(!task_finish(task, (void**)&data)) {
			log_fatal("Internal error: ires->async_task failed");
		}

		SDL_LockMutex(ires->mutex);

		if(ires->status == RES_STATUS_LOADING) {
			finish_async_load(ires, data);
		}
	}

	while(ires->status == RES_STATUS_LOADING) {
		SDL_CondWait(ires->cond, ires->mutex);
	}

	ResourceStatus status = ires->status;

	if(status == RES_STATUS_LOADED) {
		uint32_t missing_flags = want_flags & ~ires->res.flags;

		if(missing_flags) {
			uint32_t new_flags = ires->res.flags | want_flags;
			log_debug("Flags for %s at %p promoted from 0x%08x to 0x%08x", type_name(ires->res.type), (void*)ires, ires->res.flags, new_flags);
			ires->res.flags = new_flags;
		}
	}

	SDL_UnlockMutex(ires->mutex);

	return status;
}

static void unload_resource(InternalResource *ires) {
	if(wait_for_resource_load(ires, 0) == RES_STATUS_LOADED) {
		get_handler(ires->res.type)->procs.unload(ires->res.data);
	}

	SDL_DestroyCond(ires->cond);
	SDL_DestroyMutex(ires->mutex);
	free(ires);
}

static char* get_name(ResourceHandler *handler, const char *path) {
	if(handler->procs.name) {
		return handler->procs.name(path);
	}

	return resource_util_basename(handler->subdir, path);
}

static void* load_resource_async_task(void *vdata) {
	ResourceAsyncLoadData *data = vdata;

	SDL_LockMutex(data->ires->mutex);
	data->opaque = get_ires_handler(data->ires)->procs.begin_load(data->path, data->flags);
	events_emit(TE_RESOURCE_ASYNC_LOADED, 0, data->ires, data);
	SDL_UnlockMutex(data->ires->mutex);

	return data;
}

static bool resource_asyncload_handler(SDL_Event *evt, void *arg) {
	assert(is_main_thread());

	InternalResource *ires = evt->user.data1;

	SDL_LockMutex(ires->mutex);
	Task *task = ires->async_task;
	assert(!task || ires->status == RES_STATUS_LOADING);
	ires->async_task = NULL;
	SDL_UnlockMutex(ires->mutex);

	if(task == NULL) {
		return true;
	}

	ResourceAsyncLoadData *data, *verify_data;

	if(!task_finish(task, (void**)&verify_data)) {
		log_fatal("Internal error: data->ires->async_task failed");
	}

	SDL_LockMutex(ires->mutex);

	if(ires->status == RES_STATUS_LOADING) {
		data = evt->user.data2;
		assert(data == verify_data);
		finish_async_load(ires, data);
	}

	SDL_UnlockMutex(ires->mutex);

	return true;
}

static void load_resource_async(InternalResource *ires, char *path, char *name, ResourceFlags flags) {
	ResourceAsyncLoadData *data = malloc(sizeof(ResourceAsyncLoadData));

	log_debug("Loading %s '%s' asynchronously", type_name(ires->res.type), name);

	data->ires = ires;
	data->path = path;
	data->name = name;
	data->flags = flags;
	ires->async_task = taskmgr_global_submit((TaskParams) { load_resource_async_task, data });
}

void load_resource(InternalResource *ires, const char *path, const char *name, ResourceFlags flags, bool async) {
	ResourceHandler *handler = get_ires_handler(ires);
	const char *typename = type_name(handler->type);
	char *allocated_path = NULL;
	char *allocated_name = NULL;

	assert(path || name);

	if(handler->type == RES_SFX || handler->type == RES_BGM) {
		// audio stuff is always optional.
		// loading may fail if the backend failed to initialize properly, even though the resource exists.
		// this is a less than ideal situation, but it doesn't render the game unplayable.
		flags |= RESF_OPTIONAL;
	}

	if(!path) {
		path = allocated_path = handler->procs.find(name);

		if(!path) {
			if(!(flags & RESF_OPTIONAL)) {
				log_fatal("Required %s '%s' couldn't be located", typename, name);
			} else {
				log_warn("Failed to locate %s '%s'", typename, name);
			}

			ires->status = RES_STATUS_FAILED;
		}
	} else if(!name) {
		name = allocated_name = get_name(handler, path);
	}

	if(path) {
		assert(handler->procs.check(path));
	}

	if(ires->status == RES_STATUS_FAILED) {
		load_resource_finish(ires, NULL, path, name, allocated_path, allocated_name, flags);
		return;
	}

	if(async) {
		// these will be freed when loading is done
		path = allocated_path ? allocated_path : strdup(path);
		name = allocated_name ? allocated_name : strdup(name);
		load_resource_async(ires, (char*)path, (char*)name, flags);
	} else {
		load_resource_finish(ires, handler->procs.begin_load(path, flags), path, name, allocated_path, allocated_name, flags);
	}
}

static void finalize_resource(InternalResource *ires, const char *name, void *data, ResourceFlags flags, const char *source) {
	assert(name != NULL);
	assert(source != NULL);

	if(data == NULL) {
		const char *typename = type_name(ires->res.type);
		if(!(flags & RESF_OPTIONAL)) {
			log_fatal("Required %s '%s' couldn't be loaded", typename, name);
		} else {
			log_warn("Failed to load %s '%s'", typename, name);
		}
	}

	if(ires->res.type == RES_MODEL || env_get("TAISEI_NOUNLOAD", false)) {
		// FIXME: models can't be safely unloaded at runtime
		flags |= RESF_PERMANENT;
	}

	ires->res.flags = flags;
	ires->res.data = data;

	if(data) {
		log_info("Loaded %s '%s' from '%s' (%s)", type_name(ires->res.type), name, source, (flags & RESF_PERMANENT) ? "permanent" : "transient");
	}

	ires->status = data ? RES_STATUS_LOADED : RES_STATUS_FAILED;
}

static void load_resource_finish(InternalResource *ires, void *opaque, const char *path, const char *name, char *allocated_path, char *allocated_name, ResourceFlags flags) {
	void *raw = (ires->status == RES_STATUS_FAILED) ? NULL : get_ires_handler(ires)->procs.end_load(opaque, path, flags);

	name = name ? name : "<name unknown>";
	path = path ? path : "<path unknown>";

	char *sp = vfs_repr(path, true);
	finalize_resource(ires, name, raw, flags, sp ? sp : path);
	free(sp);

	free(allocated_path);
	free(allocated_name);
}

Resource* get_resource(ResourceType type, const char *name, ResourceFlags flags) {
	InternalResource *ires;
	Resource *res;

	if(flags & RESF_UNSAFE) {
		// FIXME: I'm not sure we actually need this functionality.

		ires = ht_get_unsafe(&get_handler(type)->private.mapping, name, NULL);

		if(ires != NULL && ires->status == RES_STATUS_LOADED) {
			return &ires->res;
		}
	}

	if(try_begin_load_resource(type, name, &ires)) {
		SDL_LockMutex(ires->mutex);

		if(!(flags & RESF_PRELOAD)) {
			log_warn("%s '%s' was not preloaded", type_name(type), name);

			if(env_get("TAISEI_PRELOAD_REQUIRED", false)) {
				log_fatal("Aborting due to TAISEI_PRELOAD_REQUIRED");
			}
		}

		load_resource(ires, NULL, name, flags, false);
		SDL_CondBroadcast(ires->cond);

		if(ires->status == RES_STATUS_FAILED) {
			res = NULL;
		} else {
			assert(ires->status == RES_STATUS_LOADED);
			assert(ires->res.data != NULL);
			res = &ires->res;
		}

		SDL_UnlockMutex(ires->mutex);
		return res;
	} else {
		uint32_t promotion_flags = flags & RESF_PERMANENT;
		ResourceStatus status = wait_for_resource_load(ires, promotion_flags);

		if(status == RES_STATUS_FAILED) {
			return NULL;
		}

		assert(status == RES_STATUS_LOADED);
		assert(ires->res.data != NULL);

		return &ires->res;
	}
}

void* get_resource_data(ResourceType type, const char *name, ResourceFlags flags) {
	Resource *res = get_resource(type, name, flags);

	if(res) {
		return res->data;
	}

	return NULL;
}

void preload_resource(ResourceType type, const char *name, ResourceFlags flags) {
	if(env_get("TAISEI_NOPRELOAD", false))
		return;

	InternalResource *ires;

	if(try_begin_load_resource(type, name, &ires)) {
		SDL_LockMutex(ires->mutex);
		load_resource(ires, NULL, name, flags | RESF_PRELOAD, !env_get("TAISEI_NOASYNC", false));
		SDL_UnlockMutex(ires->mutex);
	}
}

void preload_resources(ResourceType type, ResourceFlags flags, const char *firstname, ...) {
	va_list args;
	va_start(args, firstname);

	for(const char *name = firstname; name; name = va_arg(args, const char*)) {
		preload_resource(type, name, flags);
	}

	va_end(args);
}

void init_resources(void) {
	for(int i = 0; i < RES_NUMTYPES; ++i) {
		ResourceHandler *h = get_handler(i);
		alloc_handler(h);

		if(h->procs.init != NULL) {
			h->procs.init();
		}
	}

	if(!env_get("TAISEI_NOASYNC", 0)) {
		EventHandler h = {
			.proc = resource_asyncload_handler,
			.priority = EPRIO_SYSTEM,
			.event_type = MAKE_TAISEI_EVENT(TE_RESOURCE_ASYNC_LOADED),
		};

		events_register_handler(&h);
	}
}

void resource_util_strip_ext(char *path) {
	char *dot = strrchr(path, '.');
	if(dot > strrchr(path, '/'))
		*dot = 0;
}

char* resource_util_basename(const char *prefix, const char *path) {
	assert(strstartswith(path, prefix));

	char *out = strdup(path + strlen(prefix));
	resource_util_strip_ext(out);

	return out;
}

const char* resource_util_filename(const char *path) {
	char *sep = strrchr(path, '/');

	if(sep) {
		return sep + 1;
	}

	return path;
}

static void* preload_shaders(const char *path, void *arg) {
	if(!_handlers[RES_SHADER_PROGRAM]->procs.check(path)) {
		return NULL;
	}

	char *name = resource_util_basename(SHPROG_PATH_PREFIX, path);
	preload_resource(RES_SHADER_PROGRAM, name, RESF_PERMANENT);

	return NULL;
}

void* resource_for_each(ResourceType type, void* (*callback)(const char *name, Resource *res, void *arg), void *arg) {
	ht_str2ptr_ts_iter_t iter;
	ht_iter_begin(&get_handler(type)->private.mapping, &iter);
	void *result = NULL;

	while(iter.has_data) {
		InternalResource *ires = iter.value;
		char *key = iter.key;

		ht_iter_next(&iter);

		if(ires->res.data == NULL) {
			continue;
		}

		if((result = callback(key, &ires->res, arg)) != NULL) {
			break;
		}
	}

	ht_iter_end(&iter);
	return result;
}

void load_resources(void) {
	for(uint i = 0; i < RES_NUMTYPES; ++i) {
		ResourceHandler *h = get_handler(i);
		assert(h != NULL);

		if(h->procs.post_init) {
			h->procs.post_init();
		}
	}

	menu_preload();

	if(env_get("TAISEI_PRELOAD_SHADERS", 0)) {
		log_warn("Loading all shaders now due to TAISEI_PRELOAD_SHADERS");
		vfs_dir_walk(SHPROG_PATH_PREFIX, preload_shaders, NULL);
	}
}

void free_resources(bool all) {
	ht_str2ptr_ts_iter_t iter;

	for(ResourceType type = 0; type < RES_NUMTYPES; ++type) {
		ResourceHandler *handler = get_handler(type);
		InternalResource *ires;
		ht_str2ptr_ts_key_list_t *unset_list = NULL, *unset_entry;

		ht_iter_begin(&handler->private.mapping, &iter);

		for(; iter.has_data; ht_iter_next(&iter)) {
			ires = iter.value;

			if(!all && (ires->res.flags & RESF_PERMANENT)) {
				continue;
			}

			unset_entry = calloc(1, sizeof(*unset_entry));
			unset_entry->key = iter.key;
			list_push(&unset_list, unset_entry);
		}

		ht_iter_end(&iter);

		for(ht_str2ptr_ts_key_list_t *c; (c = list_pop(&unset_list));) {
			char *tmp = c->key;
			char name[strlen(tmp) + 1];
			strcpy(name, tmp);

			ires = ht_get(&handler->private.mapping, name, NULL);
			assert(ires != NULL);

			attr_unused ResourceFlags flags = ires->res.flags;

			if(!all) {
				ht_unset(&handler->private.mapping, name);
			}

			unload_resource(ires);
			free(c);

			log_debug(
				"Unloaded %s '%s' (%s)",
				type_name(type),
				name,
				(flags & RESF_PERMANENT) ? "permanent" : "transient"
			);
		}

		if(all) {
			if(handler->procs.shutdown != NULL) {
				handler->procs.shutdown();
			}

			ht_destroy(&handler->private.mapping);
		}
	}

	if(!all) {
		return;
	}

	if(!env_get("TAISEI_NOASYNC", 0)) {
		events_unregister_handler(resource_asyncload_handler);
	}
}
