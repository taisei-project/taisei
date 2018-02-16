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
#include "recolor.h"
#include "vbo.h"

#include "texture.h"
#include "animation.h"
#include "sfx.h"
#include "bgm.h"
// shader
#include "model.h"
#include "postprocess.h"
#include "sprite.h"

Resources resources = {
	.handlers = {
		[RES_TEXTURE] = &texture_res_handler,
		[RES_ANIM] = &animation_res_handler,
		[RES_SFX] = &sfx_res_handler,
		[RES_BGM] = &bgm_res_handler,
		// shader
		[RES_MODEL] = &model_res_handler,
		[RES_POSTPROCESS] = &postprocess_res_handler,
		[RES_SPRITE] = &sprite_res_handler,
	},
};

static SDL_threadID main_thread_id;

static inline ResourceHandler* get_handler(ResourceType type) {
	return *(resources.handlers + type);
}

static void alloc_handler(ResourceHandler *h) {
	h->mapping = hashtable_new_stringkeys(HT_DYNAMIC_SIZE);
	h->async_load_data = hashtable_new_stringkeys(HT_DYNAMIC_SIZE);
}

static void unload_resource(Resource *res) {
	get_handler(res->type)->procs.unload(res->data);
	free(res);
}

static const char* type_name(ResourceType type) {
	return get_handler(type)->typename;
}

Resource* insert_resource(ResourceType type, const char *name, void *data, ResourceFlags flags, const char *source) {
	assert(name != NULL);
	assert(source != NULL);

	if(data == NULL) {
		// Will only get here through some external loading mechanism (like laser snippets)

		const char *typename = type_name(type);
		if(!(flags & RESF_OPTIONAL)) {
			log_fatal("Required %s '%s' couldn't be loaded", typename, name);
		} else {
			log_warn("Failed to load %s '%s'", typename, name);
			return NULL;
		}
	}

	ResourceHandler *handler = get_handler(type);
	Resource *oldres = hashtable_get_string(handler->mapping, name);
	Resource *res = malloc(sizeof(Resource));

	if(type == RES_MODEL || getenvint("TAISEI_NOUNLOAD", false)) {
		// FIXME: models can't be safely unloaded at runtime
		flags |= RESF_PERMANENT;
	}

	res->type = handler->type;
	res->flags = flags;
	res->data = data;

	if(oldres) {
		log_warn("Replacing a previously loaded %s '%s'", type_name(type), name);
		unload_resource(oldres);
	}

	hashtable_set_string(handler->mapping, name, res);

	log_info("Loaded %s '%s' from '%s' (%s)", type_name(handler->type), name, source,
		(flags & RESF_PERMANENT) ? "permanent" : "transient");

	return res;
}

static char* get_name(ResourceHandler *handler, const char *path) {
	if(handler->procs.name) {
		return handler->procs.name(path);
	}

	return resource_util_basename(handler->subdir, path);
}

typedef struct ResourceAsyncLoadData {
	ResourceHandler *handler;
	char *path;
	char *name;
	ResourceFlags flags;
	void *opaque;
} ResourceAsyncLoadData;

static int load_resource_async_thread(void *vdata) {
	ResourceAsyncLoadData *data = vdata;

	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);
	data->opaque = data->handler->procs.begin_load(data->path, data->flags);
	events_emit(TE_RESOURCE_ASYNC_LOADED, 0, data, NULL);

	return 0;
}

static Resource* load_resource_finish(void *opaque, ResourceHandler *handler, const char *path, const char *name, char *allocated_path, char *allocated_name, ResourceFlags flags);

static bool resource_asyncload_handler(SDL_Event *evt, void *arg) {
	assert(SDL_ThreadID() == main_thread_id);

	ResourceAsyncLoadData *data = evt->user.data1;

	if(!data) {
		return true;
	}

	char name[strlen(data->name) + 1];
	strcpy(name, data->name);

	load_resource_finish(data->opaque, data->handler, data->path, data->name, data->path, data->name, data->flags);
	hashtable_unset(data->handler->async_load_data, name);
	free(data);

	return true;
}

static void load_resource_async(ResourceHandler *handler, char *path, char *name, ResourceFlags flags) {
	log_debug("Loading %s '%s' asynchronously", type_name(handler->type), name);

	ResourceAsyncLoadData *data = malloc(sizeof(ResourceAsyncLoadData));
	hashtable_set_string(handler->async_load_data, name, data);

	data->handler = handler;
	data->path = path;
	data->name = name;
	data->flags = flags;

	SDL_Thread *thread = SDL_CreateThread(load_resource_async_thread, __func__, data);

	if(thread) {
		SDL_DetachThread(thread);
	} else {
		log_warn("SDL_CreateThread() failed: %s", SDL_GetError());
		log_warn("Falling back to synchronous loading. Use TAISEI_NOASYNC=1 to suppress this warning.");
		load_resource_async_thread(data);
	}
}

static void update_async_load_state(void) {
	SDL_Event evt;
	uint32_t etype = MAKE_TAISEI_EVENT(TE_RESOURCE_ASYNC_LOADED);

	while(SDL_PeepEvents(&evt, 1, SDL_GETEVENT, etype, etype)) {
		resource_asyncload_handler(&evt, NULL);
	}
}

static bool resource_check_async_load(ResourceHandler *handler, const char *name) {
	if(SDL_ThreadID() == main_thread_id) {
		update_async_load_state();
	}

	ResourceAsyncLoadData *data = hashtable_get_string(handler->async_load_data, name);
	return data;
}

static void resource_wait_for_async_load(ResourceHandler *handler, const char *name) {
	while(resource_check_async_load(handler, name));
}

static void resource_wait_for_all_async_loads(ResourceHandler *handler) {
	char *key;

	hashtable_lock(handler->async_load_data);
	HashtableIterator *i = hashtable_iter(handler->async_load_data);
	while(hashtable_iter_next(i, (void**)&key, NULL)) {
		resource_check_async_load(handler, key);
	}
	hashtable_unlock(handler->async_load_data);
}

static Resource* load_resource(ResourceHandler *handler, const char *path, const char *name, ResourceFlags flags, bool async) {
	Resource *res;

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

			return NULL;
		}
	} else if(!name) {
		name = allocated_name = get_name(handler, path);
	}

	assert(handler->procs.check(path));

	if(async) {
		if(resource_check_async_load(handler, name)) {
			return NULL;
		}
	} else {
		resource_wait_for_async_load(handler, name);
	}

	res = hashtable_get_string(handler->mapping, name);

	if(res) {
		log_warn("%s '%s' is already loaded", typename, name);
		free(allocated_name);
		return res;
	}

	if(async) {
		// these will be freed when loading is done
		path = allocated_path ? allocated_path : strdup(path);
		name = allocated_name ? allocated_name : strdup(name);
		load_resource_async(handler, (char*)path, (char*)name, flags);
		return NULL;
	}

	return load_resource_finish(handler->procs.begin_load(path, flags), handler, path, name, allocated_path, allocated_name, flags);
}

static Resource* load_resource_finish(void *opaque, ResourceHandler *handler, const char *path, const char *name, char *allocated_path, char *allocated_name, ResourceFlags flags) {
	const char *typename = type_name(handler->type);
	void *raw = handler->procs.end_load(opaque, path, flags);

	if(!raw) {
		name = name ? name : "<name unknown>";
		path = path ? path : "<path unknown>";

		if(!(flags & RESF_OPTIONAL)) {
			log_fatal("Required %s '%s' couldn't be loaded (%s)", typename, name, path);
		} else {
			log_warn("Failed to load %s '%s' (%s)", typename, name, path);
		}

		free(allocated_path);
		free(allocated_name);

		return NULL;
	}

	char *sp = vfs_repr(path, true);
	Resource *res = insert_resource(handler->type, name, raw, flags, sp);
	free(sp);

	free(allocated_path);
	free(allocated_name);

	return res;
}

Resource* get_resource_p(ResourceType type, const char *name, ResourceFlags flags, void **out) {
	ResourceHandler *handler = get_handler(type);
	Resource *res;

	if(flags & RESF_UNSAFE) {
		res = hashtable_get_unsafe(handler->mapping, (void*)name);
		flags &= ~RESF_UNSAFE;
	} else {
		res = hashtable_get(handler->mapping, (void*)name);
	}

	if(res) {
		if(out) {
			*out = res->data;
		}

		return res;
	}

	resource_wait_for_async_load(handler, name);
	res = hashtable_get(handler->mapping, (void*)name);

	if(!res) {
		if(!(flags & RESF_PRELOAD)) {
			log_warn("%s '%s' was not preloaded", type_name(type), name);

			if(!(flags & RESF_OPTIONAL) && getenvint("TAISEI_PRELOAD_REQUIRED", false)) {
				log_fatal("Aborting due to TAISEI_PRELOAD_REQUIRED");
			}
		}

		res = load_resource(handler, NULL, name, flags, false);
	}

	if(res && flags & RESF_PERMANENT && !(res->flags & RESF_PERMANENT)) {
		log_debug("Promoted %s '%s' to permanent", type_name(type), name);
		res->flags |= RESF_PERMANENT;
	}

	if(res) {
		*out = res->data;
	} else {
		*out = NULL;
	}

	return res;
}

Resource* get_resource(ResourceType type, const char *name, ResourceFlags flags) {
	return get_resource_p(type, name, flags, NULL);
}

void preload_resource(ResourceType type, const char *name, ResourceFlags flags) {
	if(getenvint("TAISEI_NOPRELOAD", false))
		return;

	ResourceHandler *handler = get_handler(type);

	if(hashtable_get_string(handler->mapping, name) ||
		hashtable_get_string(handler->async_load_data, name)) {
		return;
	}

	load_resource(handler, NULL, name, flags | RESF_PRELOAD, !getenvint("TAISEI_NOASYNC", false));
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
	init_sdl_image();

	for(ResourceHandler **h = resources.handlers; h < resources.handlers + RES_NUMTYPES; ++h) {
		alloc_handler(*h);
	}

	main_thread_id = SDL_ThreadID();

	if(!getenvint("TAISEI_NOASYNC", 0)) {
		EventHandler h = {
			.proc = resource_asyncload_handler,
			.priority = EPRIO_SYSTEM,
			.event_type = MAKE_TAISEI_EVENT(TE_RESOURCE_ASYNC_LOADED),
		};

		events_register_handler(&h);
	}

	recolor_init();
	preload_resource(RES_SHADER, "texture_post_load", RESF_PERMANENT);
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

void load_resources(void) {
	if(glext.draw_instanced) {
		load_shader_snippets(SHA_PATH_PREFIX "laser_snippets", "laser_", RESF_PERMANENT);
	}

	menu_preload();
}

void free_resources(bool all) {
	for(ResourceType type = 0; type < RES_NUMTYPES; ++type) {
		ResourceHandler *handler = get_handler(type);

		resource_wait_for_all_async_loads(handler);

		char *name;
		Resource *res;
		ListContainer *unset_list = NULL;

		for(HashtableIterator *i = hashtable_iter(handler->mapping); hashtable_iter_next(i, (void**)&name, (void**)&res);) {
			if(!all && res->flags & RESF_PERMANENT)
				continue;

			ResourceFlags flags __attribute__((unused)) = res->flags;
			unload_resource(res);
			log_debug("Unloaded %s '%s' (%s)", type_name(type), name,
				(flags & RESF_PERMANENT) ? "permanent" : "transient"
			);

			if(!all) {
				hashtable_unset_deferred(handler->mapping, name, &unset_list);
			}
		}

		if(all) {
			hashtable_free(handler->mapping);
		} else {
			hashtable_unset_deferred_now(handler->mapping, &unset_list);
		}
	}

	if(!all) {
		return;
	}

	delete_vbo(&_vbo);
	delete_fbo_pair(&resources.fbo_pairs.bg);
	delete_fbo_pair(&resources.fbo_pairs.fg);
	delete_fbo_pair(&resources.fbo_pairs.rgba);

	if(!getenvint("TAISEI_NOASYNC", 0)) {
		events_unregister_handler(resource_asyncload_handler);
	}

	IMG_Quit();
}
