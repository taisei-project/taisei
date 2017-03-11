/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <dirent.h>
#include "resource.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "paths/native.h"
#include "config.h"
#include "taisei_err.h"
#include "video.h"

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
	ResourceTransientFunc istransient,
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
	h->istransient = istransient;
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

	printf("Loaded %s '%s' from '%s'\n", resource_type_names[handler->type], name, source);
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

	raw = handler->load(path);

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
		res = load_resource(handler, NULL, name, flags);
	}

	if(flags & RESF_PERMANENT) {
		res->flags |= RESF_PERMANENT;
	}

	return res;
}

bool istransient_filepath(const char *path) {
	char *fnstart = strrchr(path, '/');
	if (fnstart == NULL) return false;
	char *stgstart = strstr(path, "stage");
	return fnstart && stgstart && (stgstart < fnstart);
}

bool istransient_filename(const char *path) {
	char *fnstart = strrchr(path, '/');
	if (fnstart == NULL) return false;
	return strstr(fnstart, "stage");
}

void init_resources(void) {
	// hashtable sizes were carefully pulled out of my ass to reduce collisions a bit

	register_handler(
		RES_TEXTURE, TEX_PATH_PREFIX, load_texture, (ResourceUnloadFunc)free_texture, NULL, texture_path, check_texture_path, istransient_filepath, 227
	);

	register_handler(
		RES_ANIM, ANI_PATH_PREFIX, load_animation, free, animation_name, animation_path, check_animation_path, NULL, 23
	);

	register_handler(
		RES_SHADER, SHA_PATH_PREFIX, load_shader_file, unload_shader, NULL, shader_path, check_shader_path, NULL, 29
	);

	register_handler(
		RES_MODEL, MDL_PATH_PREFIX, load_model, unload_model, NULL, model_path, check_model_path, NULL, 17
	);

	register_handler(
		RES_SFX, SFX_PATH_PREFIX, load_sound, unload_sound, NULL, sound_path, check_sound_path, NULL, 16
	);

	register_handler(
		RES_BGM, BGM_PATH_PREFIX, load_music, unload_music, NULL, music_path, check_music_path, istransient_filename, 16
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

static void recurse_dir(const char *path) {
	DIR *dir = opendir(path);
	if(dir == NULL)
		errx(-1, "Can't open directory '%s'", path);

	struct dirent *dp;

	while((dp = readdir(dir)) != NULL) {
		char *filepath = strjoin(path, "/", dp->d_name, NULL);
		struct stat statbuf;

		stat(filepath, &statbuf);

		if(S_ISDIR(statbuf.st_mode) && *dp->d_name != '.') {
			recurse_dir(filepath);
		} else for(int rtype = 0; rtype < RES_NUMTYPES; ++rtype) {
			ResourceHandler *handler = get_handler(rtype);

			// NULL transient handler treats this resource type as permanent
			if((!handler->istransient || !handler->istransient(filepath)) && handler->check(filepath)) {
				load_resource(handler, filepath, NULL, 0);
			}
		}

		free(filepath);
	}

	closedir(dir);
}

static void recurse_dir_rel(const char *relpath) {
	char *path = strdup(relpath);
	strip_trailing_slashes(path);
	recurse_dir(path);
	free(path);
}

static void scan_resources(void) {
	ListContainer *visited = NULL;

	for(ResourceType type = 0; type < RES_NUMTYPES; ++type) {
		ResourceHandler *handler = get_handler(type);
		ListContainer *c;
		bool skip = false;

		for(c = visited; c; c = c->next) {
			if(!strcmp((char*)c->data, handler->subdir)) {
				skip = true;
				break;
			}
		}

		if(!skip) {
			recurse_dir_rel(handler->subdir);
			create_container(&visited)->data = handler->subdir;
		}
	}

	delete_all_elements((void**)&visited, delete_element);
}

void load_resources(void) {
	if(!getenvint("TAISEI_NOSCAN")) {
		scan_resources();
	}

	init_fonts();

	if(glext.draw_instanced) {
		load_shader_snippets("shader/laser_snippets", "laser_");
	}

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

			unload_resource(res);
			printf("Unloaded %s '%s'\n", resource_type_names[type], name);

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
