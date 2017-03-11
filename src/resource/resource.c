/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "resource.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "paths/native.h"
#include "config.h"
#include "taisei_err.h"
#include "video.h"
#include "menu/mainmenu.h"

Resources resources;

static const char *resource_type_names[] = {
	"texture",
	"animation",
	"sound",
	"bgm",
	"shader",
	"model",
};

static inline ResourceHandler* get_handler(ResourceType type) {
	return resources.handlers + type;
}

static void register_handler(
	ResourceType type,
	const char *subdir,	// trailing slash assumed
	ResourceLoadFunc load,
	ResourceUnloadFunc unload,
	ResourceNameFunc name,
	ResourceFindFunc find,
	ResourceCheckFunc check,
	size_t tablesize)
{
	assert(type >= 0 && type < RES_NUMTYPES);

	ResourceHandler *h = get_handler(type);
	h->type = type;
	h->load = load;
	h->unload = unload;
	h->name = name;
	h->find = find;
	h->check = check;
	h->mapping = hashtable_new_stringkeys(tablesize);
	strcpy(h->subdir, subdir);
}

static void unload_resource(Resource *res) {
	get_handler(res->type)->unload(res->data);
	free(res);
}

Resource* insert_resource(ResourceType type, const char *name, void *data, ResourceFlags flags, const char *source) {
	assert(name != NULL);
	assert(source != NULL);

	ResourceHandler *handler = get_handler(type);
	Resource *oldres = hashtable_get_string(handler->mapping, name);
	Resource *res = malloc(sizeof(Resource));

	if(type == RES_MODEL) {
		// FIXME: models can't be safely unloaded at runtime
		flags |= RESF_PERMANENT;
	}

	res->type = handler->type;
	res->flags = flags;
	res->data = data;

	if(oldres) {
		warnx("insert_resource(): replacing a previously loaded %s '%s'", resource_type_names[type], name);
		unload_resource(oldres);
	}

	hashtable_set_string(handler->mapping, name, res);

	printf("Loaded %s '%s' from '%s' (%s)\n", resource_type_names[handler->type], name, source,
		(flags & RESF_PERMANENT) ? "permanent" : "transient");

	return res;
}

static char* get_name(ResourceHandler *handler, const char *path) {
	if(handler->name) {
		return handler->name(path);
	}

	return resource_util_basename(handler->subdir, path);
}

static Resource* load_resource(ResourceHandler *handler, const char *path, const char *name, ResourceFlags flags) {
	Resource *res;
	void *raw;

	const char *typename = resource_type_names[handler->type];
	char *allocated_path = NULL;
	char *allocated_name = NULL;

	assert(path || name);

	if(!path) {
		path = allocated_path = handler->find(name);

		if(!path) {
			if(!(flags & RESF_OPTIONAL)) {
				errx(-1, "load_resource(): required %s '%s' couldn't be located", typename, name);
			} else {
				warnx("load_resource(): failed to locate %s '%s'", typename, name);
			}

			return NULL;
		}
	} else if(!name) {
		name = allocated_name = get_name(handler, path);
	}

	assert(handler->check(path));

	if(!(flags & RESF_OVERRIDE)) {
		res = hashtable_get_string(handler->mapping, name);

		if(res) {
			warnx("load_resource(): %s '%s' is already loaded", typename, name);
			free(allocated_name);
			return res;
		}
	}

	raw = handler->load(path, flags);

	if(!raw) {
		name = name ? name : "<name unknown>";
		path = path ? path : "<path unknown>";

		if(!(flags & RESF_OPTIONAL)) {
			errx(-1, "load_resource(): required %s '%s' couldn't be loaded (%s)", typename, name, path);
		} else {
			warnx("load_resource(): failed to load %s '%s' (%s)", typename, name, path);
		}

		free(allocated_path);
		free(allocated_name);

		return NULL;
	}

	res = insert_resource(handler->type, name, raw, flags, path);

	free(allocated_path);
	free(allocated_name);

	return res;
}

Resource* get_resource(ResourceType type, const char *name, ResourceFlags flags) {
	ResourceHandler *handler = get_handler(type);
	Resource *res = hashtable_get_string(handler->mapping, name);

	if(!res || flags & RESF_OVERRIDE) {
		if(!(flags & (RESF_PRELOAD | RESF_OVERRIDE))) {
			warnx("get_resource(): %s '%s' was not preloaded", resource_type_names[type], name);

			if(!(flags & RESF_OPTIONAL) && getenvint("TAISEI_PRELOAD_REQUIRED")) {
				warnx("Aborting due to TAISEI_PRELOAD_REQUIRED");
				abort();
			}
		}

		res = load_resource(handler, NULL, name, flags);
	}

	if(res && flags & RESF_PERMANENT && !(res->flags & RESF_PERMANENT)) {
		printf("Promoted %s '%s' to permanent\n", resource_type_names[type], name);
		res->flags |= RESF_PERMANENT;
	}

	return res;
}

void preload_resource(ResourceType type, const char *name, ResourceFlags flags) {
	if(!getenvint("TAISEI_NOPRELOAD")) {
		get_resource(type, name, flags | RESF_PRELOAD);
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
	// hashtable sizes were carefully pulled out of my ass to reduce collisions a bit

	register_handler(
		RES_TEXTURE, TEX_PATH_PREFIX, load_texture, (ResourceUnloadFunc)free_texture, NULL, texture_path, check_texture_path, 227
	);

	register_handler(
		RES_ANIM, ANI_PATH_PREFIX, load_animation, free, NULL, animation_path, check_animation_path, 23
	);

	register_handler(
		RES_SHADER, SHA_PATH_PREFIX, load_shader_file, unload_shader, NULL, shader_path, check_shader_path, 29
	);

	register_handler(
		RES_MODEL, MDL_PATH_PREFIX, load_model, unload_model, NULL, model_path, check_model_path, 17
	);

	register_handler(
		RES_SFX, SFX_PATH_PREFIX, load_sound, unload_sound, NULL, sound_path, check_sound_path, 16
	);

	register_handler(
		RES_BGM, BGM_PATH_PREFIX, load_music, unload_music, NULL, music_path, check_music_path, 16
	);
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
	init_fonts();

	if(glext.draw_instanced) {
		load_shader_snippets("shader/laser_snippets", "laser_", RESF_PERMANENT);
	}

	menu_preload();

	init_fbo(&resources.fbg[0]);
	init_fbo(&resources.fbg[1]);
	init_fbo(&resources.fsec);
}

void free_resources(bool all) {
	for(ResourceType type = 0; type < RES_NUMTYPES; ++type) {
		ResourceHandler *handler = get_handler(type);

		if(!handler->mapping)
			continue;

		char *name;
		Resource *res;

		for(HashtableIterator *i = hashtable_iter(handler->mapping); hashtable_iter_next(i, (void**)&name, (void**)&res);) {
			if(!all && res->flags & RESF_PERMANENT)
				continue;

			ResourceFlags flags = res->flags;
			unload_resource(res);
			printf("Unloaded %s '%s' (%s)\n", resource_type_names[type], name,
				(flags & RESF_PERMANENT) ? "permanent" : "transient"
			);

			if(!all) {
				hashtable_unset_deferred(handler->mapping, name);
			}
		}

		if(all) {
			hashtable_free(handler->mapping);
		} else {
			hashtable_unset_deferred_now(handler->mapping);
		}
	}

	if(!all) {
		return;
	}

	delete_vbo(&_vbo);

	delete_fbo(&resources.fbg[0]);
	delete_fbo(&resources.fbg[1]);
	delete_fbo(&resources.fsec);
}

void print_resource_hashtables(void) {
	for(ResourceType type = 0; type < RES_NUMTYPES; ++type) {
		hashtable_print_stringkeys(get_handler(type)->mapping);
	}
}
