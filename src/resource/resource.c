/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <SDL_image.h>

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

#include "renderer/common/backend.h"

ResourceHandler *_handlers[] = {
	[RES_TEXTURE] = &texture_res_handler,
	[RES_ANIM] = &animation_res_handler,
	[RES_SFX] = &sfx_res_handler,
	[RES_BGM] = &bgm_res_handler,
	[RES_MODEL] = &model_res_handler,
	[RES_POSTPROCESS] = &postprocess_res_handler,
	[RES_SPRITE] = &sprite_res_handler,

	// FIXME: these are currently handled by the renderer backend completely
	[RES_SHADER_OBJECT] = NULL,
	[RES_SHADER_PROGRAM] = NULL,
};

typedef enum ResourceStatus {
	RES_STATUS_LOADING,
	RES_STATUS_HALF_LOADED,
	RES_STATUS_LOADED,
	RES_STATUS_FAILED,
} ResourceStatus;

typedef struct InternalResource {
	Resource res;
	ResourceStatus status;
	SDL_mutex *mutex;
	SDL_cond *cond;
} InternalResource;

struct ResourceHandlerPrivate {
	Hashtable *mapping;
};

Resources resources;

static SDL_threadID main_thread_id; // TODO: move this somewhere else

static inline ResourceHandler* get_handler(ResourceType type) {
	return *(_handlers + type);
}

static inline ResourceHandler* get_ires_handler(InternalResource *ires) {
	return get_handler(ires->res.type);
}

static void alloc_handler(ResourceHandler *h) {
	assert(h != NULL);
	h->private = calloc(1, sizeof(ResourceHandlerPrivate));
	h->private->mapping = hashtable_new_stringkeys();
}

static const char* type_name(ResourceType type) {
	return get_handler(type)->typename;
}

static void* datafunc_begin_load_resource(void *arg) {
	ResourceType type = (intptr_t)arg;

	InternalResource *ires = calloc(1, sizeof(InternalResource));
	ires->res.type = type;
	ires->status = RES_STATUS_LOADING;
	ires->mutex = SDL_CreateMutex();
	ires->cond = SDL_CreateCond();

	return ires;
}

static bool try_begin_load_resource(ResourceType type, const char *name, InternalResource **out_ires) {
	ResourceHandler *handler = get_handler(type);
	return hashtable_try_set(handler->private->mapping, (char*)name, (void*)(uintptr_t)type, datafunc_begin_load_resource, (void**)out_ires);
}

static bool resource_asyncload_handler(SDL_Event *evt, void *arg);

static void finish_async_loads(void) {
	SDL_Event evt;
	uint32_t etype = MAKE_TAISEI_EVENT(TE_RESOURCE_ASYNC_LOADED);

	while(SDL_PeepEvents(&evt, 1, SDL_GETEVENT, etype, etype)) {
		resource_asyncload_handler(&evt, NULL);
	}
}

static ResourceStatus wait_for_resource_load(InternalResource *ires) {
	SDL_LockMutex(ires->mutex);

	while(ires->status == RES_STATUS_LOADING) {
		SDL_CondWait(ires->cond, ires->mutex);
	}

	if(ires->status == RES_STATUS_HALF_LOADED) {
		if(SDL_ThreadID() == main_thread_id) {
			finish_async_loads();
			assert(ires->status != RES_STATUS_HALF_LOADED);
		} else {
			do {
				SDL_CondWait(ires->cond, ires->mutex);
			} while(ires->status == RES_STATUS_HALF_LOADED);
		}
	}

	ResourceStatus status = ires->status;
	SDL_UnlockMutex(ires->mutex);

	return status;
}

static void unload_resource(InternalResource *ires) {
	if(wait_for_resource_load(ires) == RES_STATUS_LOADED) {
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

typedef struct ResourceAsyncLoadData {
	InternalResource *ires;
	char *path;
	char *name;
	ResourceFlags flags;
	void *opaque;
} ResourceAsyncLoadData;

static void* load_resource_async_task(void *vdata) {
	ResourceAsyncLoadData *data = vdata;

	SDL_LockMutex(data->ires->mutex);
	data->opaque = get_ires_handler(data->ires)->procs.begin_load(data->path, data->flags);
	data->ires->status = RES_STATUS_HALF_LOADED;
	events_emit(TE_RESOURCE_ASYNC_LOADED, 0, data, NULL);
	SDL_CondBroadcast(data->ires->cond);
	SDL_UnlockMutex(data->ires->mutex);

	return NULL;
}

static void load_resource_finish(InternalResource *ires, void *opaque, const char *path, const char *name, char *allocated_path, char *allocated_name, ResourceFlags flags);

static bool resource_asyncload_handler(SDL_Event *evt, void *arg) {
	assert(SDL_ThreadID() == main_thread_id);

	ResourceAsyncLoadData *data = evt->user.data1;

	if(!data) {
		return true;
	}

	SDL_LockMutex(data->ires->mutex);

	if(data->ires->status != RES_STATUS_HALF_LOADED) {
		SDL_UnlockMutex(data->ires->mutex);
		return true;
	}

	char name[strlen(data->name) + 1];
	strcpy(name, data->name);

	load_resource_finish(data->ires, data->opaque, data->path, data->name, data->path, data->name, data->flags);
	SDL_CondBroadcast(data->ires->cond);
	SDL_UnlockMutex(data->ires->mutex);
	free(data);

	return true;
}

static void load_resource_async(InternalResource *ires, char *path, char *name, ResourceFlags flags) {
	ResourceAsyncLoadData *data = malloc(sizeof(ResourceAsyncLoadData));

	log_debug("Loading %s '%s' asynchronously", type_name(ires->res.type), name);

	data->ires = ires;
	data->path = path;
	data->name = name;
	data->flags = flags;

	task_detach(taskmgr_global_submit((TaskParams) { load_resource_async_task, data }));
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
		ires = hashtable_get_unsafe(get_handler(type)->private->mapping, (char*)name);

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
		ResourceStatus status = wait_for_resource_load(ires);

		if(status == RES_STATUS_FAILED) {
			return NULL;
		}

		assert(status == RES_STATUS_LOADED);
		assert(ires->res.data != NULL);

		return &ires->res;
	}

	/*
	resource_wait_for_async_load(handler, name);
	res = hashtable_get(handler->mapping, (void*)name);

	if(!res) {
		if(!(flags & RESF_PRELOAD)) {
			log_warn("%s '%s' was not preloaded", type_name(type), name);

			if(env_get("TAISEI_PRELOAD_REQUIRED", false)) {
				log_fatal("Aborting due to TAISEI_PRELOAD_REQUIRED");
			}
		}

		res = load_resource(handler, NULL, name, flags, false);
	}

	if(res && (flags & RESF_PERMANENT) && !(res->flags & (RESF_PERMANENT | RESF_FAILED))) {
		log_debug("Promoted %s '%s' to permanent", type_name(type), name);
		res->flags |= RESF_PERMANENT;
	}

	return res;
	*/
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

static void init_sdl_image(void) {
	int want_flags = IMG_INIT_JPG | IMG_INIT_PNG;
	int init_flags = IMG_Init(want_flags);

	if((want_flags & init_flags) != want_flags) {
		log_warn(
			"SDL_image doesn't support some of the formats we want. "
			"Requested: %i, got: %i. "
			"Textures may fail to load",
			want_flags,
			init_flags
		);
	}
}

void init_resources(void) {
	_handlers[RES_SHADER_OBJECT] = _r_backend.res_handlers.shader_object;
	_handlers[RES_SHADER_PROGRAM] = _r_backend.res_handlers.shader_program;

	init_sdl_image();

	for(int i = 0; i < RES_NUMTYPES; ++i) {
		ResourceHandler *h = get_handler(i);
		alloc_handler(h);
	}

	main_thread_id = SDL_ThreadID();

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

struct resource_for_each_arg {
	void *(*callback)(const char *name, Resource *res, void *arg);
	void *arg;
};

static void* resource_for_each_ht_adapter(void *key, void *data, void *varg) {
	const char *name = key;
	InternalResource *ires = data;
	struct resource_for_each_arg *arg = varg;
	return arg->callback(name, &ires->res, arg->arg);
}

void* resource_for_each(ResourceType type, void* (*callback)(const char *name, Resource *res, void *arg), void *arg) {
	ResourceHandler *handler = get_handler(type);
	struct resource_for_each_arg htarg = { callback, arg };
	return hashtable_foreach(handler->private->mapping, resource_for_each_ht_adapter, &htarg);
}

void load_resources(void) {
	menu_preload();

	if(env_get("TAISEI_PRELOAD_SHADERS", 0)) {
		log_warn("Loading all shaders now due to TAISEI_PRELOAD_SHADERS");
		vfs_dir_walk(SHPROG_PATH_PREFIX, preload_shaders, NULL);
	}
}

void free_resources(bool all) {
	for(ResourceType type = 0; type < RES_NUMTYPES; ++type) {
		ResourceHandler *handler = get_handler(type);

		char *name;
		InternalResource *ires;
		ListContainer *unset_list = NULL;

		hashtable_lock(handler->private->mapping);
		for(HashtableIterator *i = hashtable_iter(handler->private->mapping); hashtable_iter_next(i, (void**)&name, (void**)&ires);) {
			if(!all && (ires->res.flags & RESF_PERMANENT)) {
				continue;
			}

			list_push(&unset_list, list_wrap_container(name));
		}
		hashtable_unlock(handler->private->mapping);

		for(ListContainer *c; (c = list_pop(&unset_list));) {
			char *tmp = c->data;
			char name[strlen(tmp) + 1];
			strcpy(name, tmp);

			ires = hashtable_get_string(handler->private->mapping, name);
			attr_unused ResourceFlags flags = ires->res.flags;

			if(!all) {
				hashtable_unset_string(handler->private->mapping, name);
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
			hashtable_free(handler->private->mapping);
			free(handler->private);
		}
	}

	if(!all) {
		return;
	}

	delete_fbo_pair(&resources.fbo_pairs.bg);
	delete_fbo_pair(&resources.fbo_pairs.fg);
	delete_fbo_pair(&resources.fbo_pairs.rgba);

	if(!env_get("TAISEI_NOASYNC", 0)) {
		events_unregister_handler(resource_asyncload_handler);
	}

	IMG_Quit();
}
