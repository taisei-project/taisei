/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "dynarray.h"
#include "hashtable.h"
#include "vfs/public.h"

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
	RES_FONT,
	RES_MATERIAL,
	RES_LOCALE,
	RES_NUMTYPES,
} ResourceType;

typedef enum ResourceFlags {
	RESF_OPTIONAL = 1,
	RESF_PRELOAD = 2,
	RESF_RELOAD = 4,

	RESF_DEFAULT = 0,
} ResourceFlags;

typedef struct ResourceLoadState ResourceLoadState;

struct ResourceLoadState {
	const char *name;
	const char *path;
	void *opaque;
	ResourceFlags flags;
};

// Converts an abstract resource name into a vfs path from which a resource with that name could be loaded.
// The path may not actually exist or be usable. The load function (see below) shall deal with such cases.
// The returned path must be free()'d.
// May return NULL on failure, but does not have to.
typedef char *(*ResourceFindProc)(const char *name);

// Tells whether the resource handler should attempt to load a file, specified by a vfs path.
typedef bool (*ResourceCheckProc)(const char *path);

// Begins loading a resource. Called on an unspecified thread.
// Must call one of the following res_load_* functions before returning to indicate status.
typedef void (*ResourceLoadProc)(ResourceLoadState *st);

// Makes `dst` refer to the resource represented by `src`.
// `src` may no longer be a valid reference to the resource after this operation.
// Resource previously represented by `dst` is destroyed in the process.
// Called on the main thread, after a resource reload successfully completes.
//
// Returns true if the operation succeeds.
// In case of failure, `dst` must not be modified, and `src` must be destroyed.
typedef bool (*ResourceTransferProc)(void *dst, void *src);

void res_load_failed(ResourceLoadState *st) attr_nonnull(1);
void res_load_finished(ResourceLoadState *st, void *res) attr_nonnull(1, 2);

// Indicates that loading must continue on the main thread.
// Schedules callback to be called on the main thread at some point in the future.
// The callback's load state parameter will contain the opaque pointer passed to this function.
// If the resource has dependencies, callback will not be called until they finish loading.
void res_load_continue_on_main(ResourceLoadState *st, ResourceLoadProc callback, void *opaque) attr_nonnull(1, 2);

// Like res_load_continue_on_main, but may be called from a worker thread.
// Use this to wait for dependencies if nothing needs to be done on the main thread.
void res_load_continue_after_dependencies(ResourceLoadState *st, ResourceLoadProc callback, void *opaque) attr_nonnull(1, 2);

// Registers another resource as a dependency and schedules it for asynchronous loading.
// Use with res_load_continue_after_dependencies/res_load_continue_on_main to wait for dependencies.
void res_load_dependency(ResourceLoadState *st, ResourceType type, const char *name);

// Like vfs_open(), but registers the path to monitor the file for changes.
// When a change is detected, the resource will be reloaded.
// This only works if the transfer proc is implemented; otherwise it's equivalent to vfs_open().
// Note that file monitoring support is not guaranteed.
SDL_IOStream *res_open_file(ResourceLoadState *st, const char *path, VFSOpenMode mode);

// Unloads a resource, freeing all allocated to it memory.
typedef void (*ResourceUnloadProc)(void *res);

// Called during resource subsystem initialization
typedef void (*ResourceInitProc)(void);

// Called after the resources and rendering subsystems have been initialized
typedef void (*ResourcePostInitProc)(void);

// Called during resource subsystem shutdown
typedef void (*ResourceShutdownProc)(void);

typedef struct ResourceHandler {
	ResourceType type;

	char *typename;
	char *subdir;

	struct {
		ResourceFindProc find;
		ResourceCheckProc check;
		ResourceLoadProc load;
		ResourceUnloadProc unload;
		ResourceTransferProc transfer;
		ResourceInitProc init;
		ResourcePostInitProc post_init;
		ResourceShutdownProc shutdown;
	} procs;

	struct {
		ht_str2ptr_ts_t mapping;
	} private;
} ResourceHandler;

typedef struct Resource {
	void *data;
	ResourceType type;
	ResourceFlags flags;
} Resource;

typedef struct ResourceGroup {
	DYNAMIC_ARRAY(void*) refs;
} ResourceGroup;

void res_init(void);
void res_post_init(void);
void res_shutdown(void);
void res_reload_all(void);
void res_purge(void);

Resource *_res_get_prehashed(ResourceType type, const char *name, hash_t hash, ResourceFlags flags) attr_nonnull_all;

INLINE void *_res_get_data_prehashed(ResourceType type, const char *name, hash_t hash, ResourceFlags flags) {
	Resource *res = _res_get_prehashed(type, name, hash, flags);
	return res ? res->data : NULL;
}

attr_nonnull_all
INLINE Resource *res_get(ResourceType type, const char *name, ResourceFlags flags) {
	return _res_get_prehashed(type, name, ht_str2ptr_hash(name), flags);
}

attr_nonnull_all
INLINE void *res_get_data(ResourceType type, const char *name, ResourceFlags flags) {
	return _res_get_data_prehashed(type, name, ht_str2ptr_hash(name), flags);
}

void *res_for_each(ResourceType type, void *(*callback)(const char *name, Resource *res, void *arg), void *arg);

void res_group_init(ResourceGroup *rg) attr_nonnull_all;
void res_group_release(ResourceGroup *rg) attr_nonnull_all;
void res_group_preload(ResourceGroup *rg, ResourceType type, ResourceFlags flags, ...)
	attr_sentinel;

void res_util_strip_ext(char *path);
char *res_util_basename(const char *prefix, const char *path);

#define DEFINE_RESOURCE_GETTER(_type, _name, _enum) \
	attr_nonnull_all attr_returns_nonnull \
	INLINE _type *_name(const char *resname) { \
		return NOT_NULL(res_get_data(_enum, resname, RESF_DEFAULT)); \
	}

#define DEFINE_OPTIONAL_RESOURCE_GETTER(_type, _name, _enum) \
	attr_nonnull_all \
	INLINE _type *_name(const char *resname) { \
		return res_get_data(_enum, resname, RESF_OPTIONAL); \
	}
