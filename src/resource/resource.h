/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

#include "global.h"

#include "texture.h"
#include "animation.h"
#include "sfx.h"
#include "bgm.h"
#include "shader.h"
#include "font.h"
#include "model.h"
#include "postprocess.h"
#include "hashtable.h"

typedef enum ResourceType {
	RES_TEXTURE,
	RES_ANIM,
	RES_SFX,
	RES_BGM,
	RES_SHADER,
	RES_MODEL,
	RES_POSTPROCESS,
	RES_NUMTYPES,
} ResourceType;

typedef enum ResourceFlags {
	RESF_OPTIONAL = 1,
	RESF_PERMANENT = 2,
	RESF_PRELOAD = 4,
} ResourceFlags;

#define RESF_DEFAULT 0

// Converts a vfs path into an abstract resource name to be used as the hashtable key.
// This method is optional, the default strategy is to take the path minus the prefix and extension.
// The returned name must be free()'d.
typedef char* (*ResourceNameFunc)(const char *path);

// Converts an abstract resource name into a vfs path from which a resource with that name could be loaded.
// The path may not actually exist or be usable. The load function (see below) shall deal with such cases.
// The returned path must be free()'d.
// May return NULL on failure, but does not have to.
typedef char* (*ResourceFindFunc)(const char *name);

// Tells whether the resource handler should attempt to load a file, specified by a vfs path.
typedef bool (*ResourceCheckFunc)(const char *path);

// Begins loading a resource specified by path.
// May be called asynchronously.
// The return value is not interpreted in any way, it's just passed to the corresponding ResourceEndLoadFunc later.
typedef void* (*ResourceBeginLoadFunc)(const char *path, unsigned int flags);

// Finishes loading a resource and returns a pointer to it.
// Will be called from the main thread only.
// On failure, must return NULL and not crash the program.
typedef void* (*ResourceEndLoadFunc)(void *opaque, const char *path, unsigned int flags);

// Unloads a resource, freeing all allocated to it memory.
typedef void (*ResourceUnloadFunc)(void *res);

typedef struct ResourceHandler {
	ResourceType type;
	ResourceNameFunc name;
	ResourceFindFunc find;
	ResourceCheckFunc check;
	ResourceBeginLoadFunc begin_load;
	ResourceEndLoadFunc end_load;
	ResourceUnloadFunc unload;
	Hashtable *mapping;
	Hashtable *async_load_data;
	char subdir[32];
} ResourceHandler;

typedef struct Resource {
	ResourceType type;
	ResourceFlags flags;

	union {
		void *data;
		Texture *texture;
		Animation *animation;
		Sound *sound;
		Music *music;
		Shader *shader;
		Model *model;
		PostprocessShader *postprocess;
	};
} Resource;

typedef struct Resources {
	ResourceHandler handlers[RES_NUMTYPES];
	PostprocessShader *stage_postprocess;
	FontRenderer fontren;

	struct {
		FBO bg[2];
		FBO fg[2];
		FBO rgba[2]; // XXX: do we actually need 2 RGBA FBOs?
	} fbo;
} Resources;

extern Resources resources;

void init_resources(void);
void load_resources(void);
void free_resources(bool all);

Resource* get_resource(ResourceType type, const char *name, ResourceFlags flags);
Resource* insert_resource(ResourceType type, const char *name, void *data, ResourceFlags flags, const char *source);
void preload_resource(ResourceType type, const char *name, ResourceFlags flags);
void preload_resources(ResourceType type, ResourceFlags flags, const char *firstname, ...) __attribute__((sentinel));

void resource_util_strip_ext(char *path);
char* resource_util_basename(const char *prefix, const char *path);
const char* resource_util_filename(const char *path);

bool resource_sdl_event(SDL_Event *evt);
