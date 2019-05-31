/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_resource_resource_h
#define IGUARD_resource_resource_h

#include "taisei.h"

#include "hashtable.h"

// #define RESOURCE_TYPE(idname, capname, fullname, pathprefix)
#define RESOURCE_TYPES \
	RESOURCE_TYPE(texture,     TEXTURE,     "texture",        "res/gfx/") \
	RESOURCE_TYPE(animation,   ANIMATION,   "animation",      "res/gfx/") \
	RESOURCE_TYPE(sound,       SOUND,       "sound",          "res/sfx/") \
	RESOURCE_TYPE(music,       MUSIC,       "music",          "res/bgm/") \
	RESOURCE_TYPE(musicmeta,   MUSICMETA,   "music metadata", "res/bgm/") \
	RESOURCE_TYPE(shaderobj,   SHADEROBJ,   "shader object",  "res/shader/") \
	RESOURCE_TYPE(shaderprog,  SHADERPROG,  "shader program", "res/shader/") \
	RESOURCE_TYPE(model,       MODEL,       "model",          "res/models/") \
	RESOURCE_TYPE(postprocess, POSTPROCESS, "postprocess",    "res/shader/") \
	RESOURCE_TYPE(sprite,      SPRITE,      "sprite",         "res/gfx/") \
	RESOURCE_TYPE(font,        FONT,        "font",           "res/fonts/") \

typedef enum ResourceType {
	#define RESOURCE_TYPE(idname, capname, fullname, pathprefix) RES_##capname,
	RESOURCE_TYPES
	#undef RESOURCE_TYPE

	RES_NUMTYPES,
} ResourceType;

#define RESOURCE_TYPE(idname, capname, fullname, pathprefix) \
	attr_unused static const char *const RES_PATHPREFIX_##capname = pathprefix;
RESOURCE_TYPES
#undef RESOURCE_TYPE

typedef enum ResourceFlags {
	RESF_OPTIONAL = 1,  /// don't crash in case the resource fails to load
	RESF_LAZY = 2,  /// load on first use only, synchronously

	/// internal flags for refs, do not use.
	RESF_WEAK = 4,  /// only refers to a resource, has no effect on its reference count
	RESF_ALIEN = (RESF_WEAK | RESF_LAZY),  /// wraps some external data, not managed by the resource system

	RESF_INTERNAL_REF_FLAG_BITS = RESF_WEAK,
} ResourceFlags;

typedef enum ResourceStatus {
	RES_STATUS_NOT_LOADED,
	RES_STATUS_LOADING,
	RES_STATUS_LOADED,
	RES_STATUS_FAILED,
	RES_STATUS_EXTERNAL,
} ResourceStatus;

#define RESF_DEFAULT 0

// Information about a resource being loaded.
// This struct is passed to ResourceBeginLoadProc and ResourceEndLoadProc (see below).
typedef struct ResourceLoadInfo {
	const char *name;    // canonical name of the resource being loaded
	const char *path;    // VFS file the data is to be loaded from
	ResourceType type;   // type of the resource being loaded
	ResourceFlags flags; // requested load flags; dependencies should be loaded with the same flags
} ResourceLoadInfo;

// Converts a canonical resource name into a vfs path from which a resource with that name could be loaded.
// The path may not actually exist or be usable. The load function (see below) shall deal with such cases.
// The returned path must be free()'d.
// May return NULL on failure, but does not have to.
typedef char *(*ResourceFindProc)(const char *name);

// Tells whether the resource handler should attempt to load a file, specified by a vfs path.
// Must return true for any path returned from ResourceFindProc.
typedef bool (*ResourceCheckProc)(const char *path);

// Begins loading a resource specified by ResourceLoadInfo.
// May be called asynchronously.
// On failure, must return NULL and not crash the program.
// Any other return value will eventually be passed to the corresponding ResourceEndLoadProc function (see below).
typedef void *(*ResourceBeginLoadProc)(ResourceLoadInfo loadinfo);

// Finishes loading a resource and returns a pointer to its data.
// Will be called from the main thread only.
// `opaque` has the same value as returned by ResourceBeginLoadProc.
// On failure, must return NULL and not crash the program.
// This function is optional. If missing, the value of `opaque` is interpreted as a pointer to the resource data.
// You should only provide this function if the load must be finalized on the main thread.
typedef void *(*ResourceEndLoadProc)(ResourceLoadInfo loadinfo, void *opaque);

// Unloads a resource, freeing all allocated to it memory.
typedef void (*ResourceUnloadProc)(void *res);

// Called during resource subsystem initialization
typedef void (*ResourceInitProc)(void);

// Called after the resources and rendering subsystems have been initialized
typedef void (*ResourcePostInitProc)(void);

// Called before resource subsystem shutdown
typedef void (*ResourcePreShutdownProc)(void);

// Called during resource subsystem shutdown; no resource APIs can be used here
typedef void (*ResourceShutdownProc)(void);

typedef struct ResourceHandler {
	/*
	const char *typename;
	const char *typeid;
	const char *subdir;
	*/
	ResourceType type;

	struct {
		ResourceFindProc find;
		ResourceCheckProc check;
		ResourceBeginLoadProc begin_load;
		ResourceEndLoadProc end_load;
		ResourceUnloadProc unload;
		ResourceInitProc init;
		ResourcePostInitProc post_init;
		ResourceShutdownProc shutdown;
		ResourcePreShutdownProc pre_shutdown;
	} procs;

	struct {
		ht_str2ptr_ts_t mapping;
	} private;

} ResourceHandler;

#define REF_POINTER_MIN_ALIGNMENT 8

#if TAISEI_BUILDCONF_RESREF_TYPE == TAISEI_BUILDCONF_RESREF_TYPE_X64 || TAISEI_BUILDCONF_RESREF_TYPE == TAISEI_BUILDCONF_RESREF_TYPE_AARCH64
#define RESREF_LAYOUT_TAGGED64
#elif TAISEI_BUILDCONF_RESREF_TYPE == TAISEI_BUILDCONF_RESREF_TYPE_PORTABLE
#define RESREF_LAYOUT_PORTABLE
#else
#error Invalid value of TAISEI_BUILDCONF_RESREF_TYPE
#endif

typedef struct ResourceRef {
	struct {
#if defined(RESREF_LAYOUT_TAGGED64)
		union {
			uintptr_t handle;
			void *handle_as_ptr;
		};
#elif defined(RESREF_LAYOUT_PORTABLE)
		void *ptr;
		uint16_t flags;
		uint16_t type;
#endif
	} _internal;

#ifdef DEBUG
	void *_beacon;  // holds a dummy 1-byte heap allocation; this is a cheap way to detect some ref leaks with ASan/LeakSan
#endif
} ResourceRef;

#define RES_INVALID_REF ((ResourceRef) { 0 })

typedef uintptr_t res_id_t;

void res_init(void);
void res_post_init(void);
void res_shutdown(void);
void res_purge(uint limit, uint timegap);

ResourceHandler *res_type_handler(ResourceType type);
const char *res_type_idname(ResourceType type);
const char *res_type_fullname(ResourceType type);
const char *res_type_pathprefix(ResourceType type);

ResourceRef res_ref(ResourceType type, const char *name, ResourceFlags flags) attr_nonnull(2);
ResourceRef res_ref_copy(ResourceRef ref);
ResourceRef res_ref_copy_weak(ResourceRef ref);
ResourceRef res_ref_wrap_external_data(ResourceType type, void *data) attr_nonnull(2);
void res_unref(ResourceRef *ref) attr_nonnull(1);
void res_unref_if_valid(ResourceRef *ref) attr_nonnull(1);  // only use this one for refs that may be *intentionally* invalid
void *res_ref_data(ResourceRef ref);
ResourceType res_ref_type(ResourceRef ref);
ResourceStatus res_ref_status(ResourceRef ref);
ResourceStatus res_ref_wait_ready(ResourceRef ref);
const char *res_ref_name(ResourceRef ref);
bool res_ref_is_valid(ResourceRef ref);
res_id_t res_ref_id(ResourceRef ref);
bool res_refs_are_equivalent(ResourceRef ref1, ResourceRef ref2);

static inline attr_must_inline void *res_ref_data_or_null(ResourceRef ref) {
	return res_ref_is_valid(ref) ? res_ref_data(ref) : NULL;
}

void *res_for_each(ResourceType type, void *(*callback)(const char *name, void *res, void *arg), void *arg) attr_nonnull(2);

void res_util_strip_ext(char *path) attr_nonnull(1);
char *res_util_basename(const char *prefix, const char *path) attr_nonnull(1, 2) attr_nodiscard attr_returns_nonnull;

//
// HACK: All the following is a temporary measure to not break too much code relying on the old preload/resource lifetime semantics.
// Lua code will have no access to these, instead relying on refs and GC transparently.
// In C, working has with refs must be explicit. Rewriting a ton of code in that obtuse style, only to eventually port it to Lua later anyway, isn't worth it to say the least.
//

typedef struct ResourceRefGroup {
	ResourceRef *refs;
	uint num_allocated;
	uint num_used;
} ResourceRefGroup;

void *res_get_data(ResourceType type, const char *name, ResourceFlags flags) attr_nonnull(2);
void res_group_init(ResourceRefGroup *group, uint init_size) attr_nonnull(1);
void res_group_destroy(ResourceRefGroup *group);
void res_group_add_ref(ResourceRefGroup *group, ResourceRef ref) attr_nonnull(1);
void res_group_multi_add(ResourceRefGroup *group, ResourceType type, ResourceFlags flags, const char *firstname, ...) attr_nonnull(1, 4) attr_sentinel;
bool res_group_is_ready(ResourceRefGroup *group) attr_nonnull(1);
void res_group_wait_ready(ResourceRefGroup *group) attr_nonnull(1);

#endif // IGUARD_resource_resource_h
