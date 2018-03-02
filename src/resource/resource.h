/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "hashtable.h"
// #include "fbo.h"

typedef enum ResourceType {
	RES_TEXTURE,
	RES_ANIM,
	RES_SFX,
	RES_BGM,
	RES_SHADER_OBJECT,
	RES_SHADER_PROGRAM,
	RES_MODEL,
	RES_POSTPROCESS,
	RES_SPRITE,
	RES_NUMTYPES,
} ResourceType;

typedef enum ResourceFlags {
	RESF_OPTIONAL = 1,
	RESF_PERMANENT = 2,
	RESF_PRELOAD = 4,
	RESF_UNSAFE = 8,
} ResourceFlags;

#define RESF_DEFAULT 0

// Converts a vfs path into an abstract resource name to be used as the hashtable key.
// This method is optional, the default strategy is to take the path minus the prefix and extension.
// The returned name must be free()'d.
typedef char* (*ResourceNameProc)(const char *path);

// Converts an abstract resource name into a vfs path from which a resource with that name could be loaded.
// The path may not actually exist or be usable. The load function (see below) shall deal with such cases.
// The returned path must be free()'d.
// May return NULL on failure, but does not have to.
typedef char* (*ResourceFindProc)(const char *name);

// Tells whether the resource handler should attempt to load a file, specified by a vfs path.
typedef bool (*ResourceCheckProc)(const char *path);

// Begins loading a resource specified by path.
// May be called asynchronously.
// The return value is not interpreted in any way, it's just passed to the corresponding ResourceEndLoadProc later.
typedef void* (*ResourceBeginLoadProc)(const char *path, uint flags);

// Finishes loading a resource and returns a pointer to it.
// Will be called from the main thread only.
// On failure, must return NULL and not crash the program.
typedef void* (*ResourceEndLoadProc)(void *opaque, const char *path, uint flags);

// Unloads a resource, freeing all allocated to it memory.
typedef void (*ResourceUnloadProc)(void *res);

typedef struct ResourceHandler {
	ResourceType type;

	char *typename;
	char *subdir;

	struct {
		ResourceNameProc name;
		ResourceFindProc find;
		ResourceCheckProc check;
		ResourceBeginLoadProc begin_load;
		ResourceEndLoadProc end_load;
		ResourceUnloadProc unload;
	} procs;

	Hashtable *mapping;
	Hashtable *async_load_data;
} ResourceHandler;

typedef struct Resource {
	ResourceType type;
	ResourceFlags flags;
	void *data;
} Resource;

void init_resources(void);
void load_resources(void);
void free_resources(bool all);

Resource* get_resource(ResourceType type, const char *name, ResourceFlags flags);
void* get_resource_data(ResourceType type, const char *name, ResourceFlags flags);
Resource* insert_resource(ResourceType type, const char *name, void *data, ResourceFlags flags, const char *source);
void preload_resource(ResourceType type, const char *name, ResourceFlags flags);
void preload_resources(ResourceType type, ResourceFlags flags, const char *firstname, ...) attr_sentinel;
Hashtable* get_resource_table(ResourceType type);

void resource_util_strip_ext(char *path);
char* resource_util_basename(const char *prefix, const char *path);
const char* resource_util_filename(const char *path);
