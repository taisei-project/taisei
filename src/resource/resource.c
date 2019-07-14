/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

/* BEGIN Headers */

#include "resource.h"
#include "config.h"
#include "video.h"
#include "events.h"
#include "taskmanager.h"

#include "texture.h"
#include "animation.h"
#include "sfx.h"
#include "bgm.h"
#include "bgm_metadata.h"
#include "shader_object.h"
#include "shader_program.h"
#include "model.h"
#include "postprocess.h"
#include "sprite.h"
#include "font.h"

#include "renderer/common/backend.h"

/* END Headers */

/* BEGIN Declarations */

static ResourceHandler *_handlers[] = {
	[RES_TEXTURE] = &texture_res_handler,
	[RES_ANIMATION] = &animation_res_handler,
	[RES_SOUND] = &sfx_res_handler,
	[RES_MUSIC] = &bgm_res_handler,
	[RES_MUSICMETA] = &bgm_metadata_res_handler,
	[RES_MODEL] = &model_res_handler,
	[RES_POSTPROCESS] = &postprocess_res_handler,
	[RES_SPRITE] = &sprite_res_handler,
	[RES_FONT] = &font_res_handler,
	[RES_SHADEROBJ] = &shader_object_res_handler,
	[RES_SHADERPROG] = &shader_program_res_handler,
};

static struct ResourceTypeInfo {
	const char *idname;
	const char *fullname;
	const char *pathprefix;
} res_type_info[] = {
#define RESOURCE_TYPE(idname, capname, fullname, pathprefix) \
	[RES_##capname] = { #idname, fullname, pathprefix },
	RESOURCE_TYPES
#undef RESOURCE_TYPE
};

static_assert(ARRAY_SIZE(_handlers) == RES_NUMTYPES, "");
static_assert(ARRAY_SIZE(res_type_info) == RES_NUMTYPES, "");

typedef struct Resource {
	LIST_INTERFACE(struct Resource);
	Task *async_task;
	SDL_mutex *mutex;
	SDL_cond *cond;
	void *data;
	char *name;
	SDL_atomic_t refcount;
	uint16_t reftime;
	ResourceType type : 6;
	ResourceFlags flags : 6;
	ResourceStatus status : 4;
} Resource;

typedef struct ResourceAsyncLoadData {
	Resource *res;
	char *path;
	void *opaque;
} ResourceAsyncLoadData;

static struct {
	LIST_ANCHOR(Resource) list;
	SDL_SpinLock lock;
	int count;
} purgatory;

static uint32_t purge_timer;

#define ENABLE_REF_DEBUG 0

#if ENABLE_REF_DEBUG
	#define REF_DEBUG(...) log_debug(__VA_ARGS__)
#else
	#define REF_DEBUG(...) ((void)0)
#endif

/* END Declarations */

/* BEGIN Helpers */

ResourceHandler *res_type_handler(ResourceType type) {
	assert((uint)type < RES_NUMTYPES);
	return _handlers[type];
}

static inline ResourceHandler *get_ires_handler(Resource *res) {
	return res_type_handler(res->type);
}

static void alloc_handler(ResourceHandler *h) {
	assert(h != NULL);
	ht_create(&h->private.mapping);
}

const char *res_type_idname(ResourceType type) {
	assert((uint)type < RES_NUMTYPES);
	return res_type_info[type].idname;
}

const char *res_type_fullname(ResourceType type) {
	assert((uint)type < RES_NUMTYPES);
	return res_type_info[type].fullname;
}

const char *res_type_pathprefix(ResourceType type) {
	assert((uint)type < RES_NUMTYPES);
	return res_type_info[type].pathprefix;
}

void res_util_strip_ext(char *path) {
	char *dot = strrchr(path, '.');
	if(dot > strrchr(path, '/'))
		*dot = 0;
}

char *res_util_basename(const char *prefix, const char *path) {
	assert(strstartswith(path, prefix));

	char *out = strdup(path + strlen(prefix));
	res_util_strip_ext(out);

	return out;
}

void *res_for_each(ResourceType type, void* (*callback)(const char *name, void *res, void *arg), void *arg) {
	ht_str2ptr_ts_iter_t iter;
	ht_iter_begin(&res_type_handler(type)->private.mapping, &iter);
	void *result = NULL;

	while(iter.has_data) {
		Resource *res = iter.value;
		char *key = iter.key;

		ht_iter_next(&iter);

		if(res->data == NULL) {
			continue;
		}

		if((result = callback(key, res->data, arg)) != NULL) {
			break;
		}
	}

	ht_iter_end(&iter);
	return result;
}

/* END Helpers */

/* BEGIN Purgatory */

attr_unused attr_must_inline
static inline void purgatory_check_consistency(void) {
	assert(purgatory.count >= 0);
	assert((purgatory.list.first && purgatory.list.last) || (!purgatory.list.first && !purgatory.list.last));
	assert(purgatory.list.first || !purgatory.count);
	assert((bool)(purgatory.count) == (bool)(purgatory.list.first));
	assert(purgatory.count == 0 || purgatory.list.first);
	assert(purgatory.count == 1 || purgatory.count == 0 || purgatory.list.first != purgatory.list.last);
}

#if ENABLE_REF_DEBUG

#define purgatory_lock() purgatory_lock(_DEBUG_INFO_)
static inline attr_must_inline void (purgatory_lock)(DebugInfo dbg) {
	log_debug("PreLock @ %s:%s:%i", dbg.file, dbg.func, dbg.line);
	SDL_AtomicLock(&purgatory.lock);
	log_debug("PostLock @ %s:%s:%i", dbg.file, dbg.func, dbg.line);
}

#define purgatory_unlock() purgatory_unlock(_DEBUG_INFO_)
static inline attr_must_inline void (purgatory_unlock)(DebugInfo dbg) {
	log_debug("PreUnLock @ %s:%s:%i", dbg.file, dbg.func, dbg.line);
	SDL_AtomicUnlock(&purgatory.lock);
	log_debug("PostUnLock @ %s:%s:%i", dbg.file, dbg.func, dbg.line);
}

#else

#define purgatory_lock() SDL_AtomicLock(&purgatory.lock)
#define purgatory_unlock() SDL_AtomicUnlock(&purgatory.lock)

#endif

static int res_purgatory_prio(List *lres) {
	Resource *res = (Resource*)lres;
	return res->reftime;
}

static void purgatory_add(Resource *res) {
	purgatory_check_consistency();
	// alist_append(&purgatory.list, res);
	alist_insert_at_priority_tail(&purgatory.list, res, res->reftime, res_purgatory_prio);
	++purgatory.count;
	REF_DEBUG("Purgatory ADD: %s %s, cnt: %i, prev: %p, next: %p, phead: %p", res_type_idname(res->type), res->name, purgatory.count, (void*)res->prev, (void*)res->next, (void*)purgatory.list.first);
	purgatory_check_consistency();
}

static inline bool in_purgatory(Resource *res) {
	return res->next || res->prev || purgatory.list.first == res;
}

static void purgatory_remove(Resource *res) {
	purgatory_check_consistency();
	if(in_purgatory(res)) {
		REF_DEBUG("Purgatory REMOVE: %s %s, cnt: %i, prev: %p, next: %p, phead: %p", res_type_idname(res->type), res->name, purgatory.count, (void*)res->prev, (void*)res->next, (void*)purgatory.list.first);
		alist_unlink(&purgatory.list, res);
		res->next = res->prev = NULL;
		--purgatory.count;
	}  else {
		REF_DEBUG("Purgatory IGNORED REMOVE: %s %s, cnt: %i, prev: %p, next: %p, phead: %p", res_type_idname(res->type), res->name, purgatory.count, (void*)res->prev, (void*)res->next, (void*)purgatory.list.first);
	}
	purgatory_check_consistency();
}

/* END Purgatory */

/* BEGIN Atomic refcount routines */

static int res_inc_refcount(Resource *res) {
	int prev = SDL_AtomicAdd(&res->refcount, 1);
	REF_DEBUG("%s %s [%i  -->  %i]", res_type_idname(res->type), res->name, prev, prev + 1);

	if(prev == 0) {
		purgatory_lock();
		purgatory_remove(res);
		purgatory_unlock();
	}

	return prev;
}

static bool res_inc_refcount_if_zero(Resource *res) {
	if(SDL_AtomicCAS(&res->refcount, 0, 1)) {
		REF_DEBUG("%s %s [0  -->  1]", res_type_idname(res->type), res->name);

		purgatory_lock();
		purgatory_remove(res);
		purgatory_unlock();

		log_warn("%s `%s` was not preloaded and will never be unloaded", res_type_fullname(res->type), res->name);

		if(env_get("TAISEI_PRELOAD_REQUIRED", false)) {
			log_fatal("Aborting due to TAISEI_PRELOAD_REQUIRED");
		}

		return true;
	}

	return false;
}

static int res_dec_refcount(Resource *res) {
	int prev = SDL_AtomicAdd(&res->refcount, -1);
	REF_DEBUG("%s %s [%i  -->  %i]", res_type_idname(res->type), res->name, prev, prev - 1);

	if(prev == 1) {
		purgatory_lock();
		purgatory_add(res);
		purgatory_unlock();
	} else {
		assert(prev > 1);
	}

	return prev;
}

/* END Atomic refcount routines */

/* BEGIN Core */

struct valfunc_arg {
	const char *name;
	ResourceType type;
	ResourceFlags flags;
};

static void *valfunc_allocate_resource(void *arg) {
	struct valfunc_arg *a = arg;

	Resource *res = calloc(1, sizeof(Resource));
	res->type = a->type;
	res->status = RES_STATUS_NOT_LOADED;
	res->mutex = SDL_CreateMutex();
	res->cond = SDL_CreateCond();
	res->name = strdup(a->name);
	res->flags = a->flags;

	if(res->type == RES_SOUND || res->type == RES_MUSIC) {
		// audio stuff is always optional.
		// loading may fail if the backend failed to initialize properly, even though the resource exists.
		// this is a less than ideal situation, but it doesn't render the game unplayable.
		res->flags |= RESF_OPTIONAL;
	}

	return res;
}

static bool try_allocate_resource(ResourceType type, const char *name, ResourceFlags flags, Resource **out_ires) {
	ResourceHandler *handler = res_type_handler(type);
	struct valfunc_arg arg = { name, type, flags };
	return ht_try_set(&handler->private.mapping, name, &arg, valfunc_allocate_resource, (void**)out_ires);
}

static void load_resource_finish(Resource *res, void *opaque, char *allocated_path);

static void finish_async_load(Resource *res, ResourceAsyncLoadData *data) {
	assert(res == data->res);
	assert(res->status == RES_STATUS_LOADING);
	load_resource_finish(res, data->opaque, data->path);
	SDL_CondBroadcast(data->res->cond);
	assert(res->status != RES_STATUS_LOADING);
	free(data);
}

static inline attr_must_inline ResourceLoadInfo make_load_info(Resource *res, const char *path) {
	ResourceLoadInfo li;
	li.name = res->name;
	li.path = path;
	li.type = res->type;
	li.flags = res->flags;
	return li;
}

static void *load_resource_async_task(void *vdata) {
	ResourceAsyncLoadData *data = vdata;
	Resource *res = data->res;
	ResourceHandler *handler = get_ires_handler(res);

	SDL_LockMutex(res->mutex);

	data->opaque = handler->procs.begin_load(make_load_info(res, data->path));

	if(handler->procs.end_load) {
		events_emit(TE_RESOURCE_ASYNC_LOADED, 0, res, data);
	} else {
		finish_async_load(res, data);
		task_detach(res->async_task);
		res->async_task= NULL;
		data = NULL;
	}

	SDL_UnlockMutex(res->mutex);

	return data;
}

static bool resource_asyncload_handler(SDL_Event *evt, void *arg) {
	assert(is_main_thread());

	Resource *res = evt->user.data1;

	SDL_LockMutex(res->mutex);
	Task *task = res->async_task;
	assert(!task || res->status == RES_STATUS_LOADING);
	res->async_task = NULL;
	SDL_UnlockMutex(res->mutex);

	if(task == NULL) {
		return true;
	}

	ResourceAsyncLoadData *data, *verify_data;

	if(!task_finish(task, (void**)&verify_data)) {
		log_fatal("Internal error: data->res->async_task failed");
	}

	SDL_LockMutex(res->mutex);

	if(res->status == RES_STATUS_LOADING) {
		data = evt->user.data2;
		assert(data == verify_data);
		finish_async_load(res, data);
	}

	SDL_UnlockMutex(res->mutex);

	return true;
}

static void load_resource_async(Resource *res, char *path) {
	ResourceAsyncLoadData *data = malloc(sizeof(ResourceAsyncLoadData));

	log_debug("Loading %s `%s` asynchronously", res_type_fullname(res->type), res->name);

	data->res = res;
	data->path = path;
	res->async_task = taskmgr_global_submit((TaskParams) { load_resource_async_task, data });
}

attr_nonnull(1)
static void load_resource(Resource *res, bool async) {
	assert(res->status == RES_STATUS_NOT_LOADED);
	res->status = RES_STATUS_LOADING;

	ResourceHandler *handler = get_ires_handler(res);
	const char *typename = res_type_fullname(handler->type);
	const char *name = res->name;
	char *path = NULL;

	path = handler->procs.find(name);

	if(!path) {
		if(!(res->flags & RESF_OPTIONAL)) {
			log_fatal("Failed to locate required %s `%s`", typename, name);
		} else {
			log_error("Failed to locate %s `%s`", typename, name);
		}

		res->status = RES_STATUS_FAILED;
	} else {
		assert(handler->procs.check(path));
	}

	if(res->status == RES_STATUS_FAILED) {
		load_resource_finish(res, NULL, path);
		return;
	}

	if(async) {
		// path will be freed when loading is done
		load_resource_async(res, path);
	} else {
		load_resource_finish(res, handler->procs.begin_load(make_load_info(res, path)), path);
	}
}

attr_nonnull(1)
static void finalize_resource(Resource *res, void *data, const char *source) {
	const char *name = res->name;

	if(data == NULL) {
		const char *typename = res_type_fullname(res->type);
		if(!(res->flags & RESF_OPTIONAL)) {
			log_fatal("Required %s `%s` couldn't be loaded", typename, name);
		} else {
			log_error("Failed to load %s `%s`", typename, name);
		}
	}

	if(res->type == RES_MODEL || env_get("TAISEI_NOUNLOAD", false)) {
		// FIXME: models can't be safely unloaded at runtime
		res_inc_refcount(res);
	}

	res->data = data;

	if(data) {
		assert(source != NULL);
		char *sp = vfs_syspath(source);
		log_info("Loaded %s `%s` from `%s`", res_type_fullname(res->type), name, sp ? sp : source);
		free(sp);
	}

	res->status = data ? RES_STATUS_LOADED : RES_STATUS_FAILED;
}

static void load_resource_finish(Resource *res, void *opaque, char *allocated_path) {
	ResourceHandler *handler = get_ires_handler(res);
	ResourceEndLoadProc endload;

	if(opaque == NULL) {
		res->status = RES_STATUS_FAILED;
	}

	void *raw;

	if(res->status == RES_STATUS_FAILED) {
		raw = NULL;
	} else if((endload = handler->procs.end_load)) {
		raw = endload(make_load_info(res, allocated_path), opaque);
	} else {
		raw = opaque;
	}

	finalize_resource(res, raw, allocated_path);
	free(allocated_path);
}

static void update_flags(Resource *res, ResourceFlags flags) {
	if(!(flags & RESF_OPTIONAL) && (res->flags & RESF_OPTIONAL)) {
		SDL_LockMutex(res->mutex);
		if(res->flags & RESF_OPTIONAL) {
			res->flags &= ~RESF_OPTIONAL;
			log_debug("RESF_OPTIONAL stripped from %s %s", res_type_fullname(res->type), res->name);
		}
		SDL_UnlockMutex(res->mutex);
	}
}

static void begin_load_resource(Resource *res, bool force_sync) {
	if(res->status == RES_STATUS_NOT_LOADED) {
		SDL_LockMutex(res->mutex);
		if(res->status == RES_STATUS_NOT_LOADED) {
			bool async = !(force_sync || env_get("TAISEI_NOASYNC", false));
			load_resource(res, async);
			if(!async) {
				SDL_CondBroadcast(res->cond);
			}
		}
		SDL_UnlockMutex(res->mutex);
	}
}

static ResourceStatus wait_for_resource_load(Resource *res) {
	SDL_LockMutex(res->mutex);

	if(res->async_task != NULL && is_main_thread()) {
		assert(res->status == RES_STATUS_LOADING);

		ResourceAsyncLoadData *data;
		Task *task = res->async_task;
		res->async_task = NULL;

		SDL_UnlockMutex(res->mutex);

		if(!task_finish(task, (void**)&data)) {
			log_fatal("Internal error: res->async_task failed");
		}

		SDL_LockMutex(res->mutex);

		if(res->status == RES_STATUS_LOADING) {
			finish_async_load(res, data);
		}
	}

	while(res->status == RES_STATUS_LOADING) {
		SDL_CondWait(res->cond, res->mutex);
	}

	ResourceStatus status = res->status;

	SDL_UnlockMutex(res->mutex);

	return status;
}

static void unload_resource(Resource *res) {
	if(wait_for_resource_load(res) == RES_STATUS_LOADED) {
		SDL_LockMutex(res->mutex);
		if(res->status == RES_STATUS_LOADED) {
			res_type_handler(res->type)->procs.unload(res->data);
			res->flags = 0;
			res->status = RES_STATUS_NOT_LOADED;
			res->data = NULL;
			log_info("Unloaded %s `%s`", res_type_fullname(res->type), res->name);
		}
		SDL_UnlockMutex(res->mutex);
		if(in_purgatory(res)) {
			purgatory_lock();
			purgatory_remove(res);
			purgatory_unlock();
		}
	}
}

static void free_resource(Resource *res) {
	assert(res->status == RES_STATUS_NOT_LOADED || res->status == RES_STATUS_FAILED);
	assert(res->data == NULL);
	SDL_DestroyCond(res->cond);
	SDL_DestroyMutex(res->mutex);
	free(res->name);
	free(res);
}

/* END Core */

/* BEGIN Refs */

/* BEGIN Needlessly complicated platform-specific madness */

INLINE void _ref_set(ResourceRef *ref, void *ptr, uintptr_t type, uintptr_t flags);
INLINE void *_ref_get_ptr(ResourceRef *ref);
INLINE uintptr_t _ref_get_type(ResourceRef *ref);
INLINE uintptr_t _ref_get_flags(ResourceRef *ref);
INLINE uintptr_t _ref_test_flags(ResourceRef *ref, uintptr_t flags);

#if defined(RESREF_LAYOUT_TAGGED64)

// tagged pointers because reasons

#define REF_FLAGS_MASK   ((uintptr_t)(REF_POINTER_MIN_ALIGNMENT - 1))

#if TAISEI_BUILDCONF_RESREF_TYPE == TAISEI_BUILDCONF_RESREF_TYPE_X64
#define REF_TYPE_MASK    (0xff00000000000000LLU)
#define REF_ADDRESS_MASK (0x0000ffffffffffffLLU & ~REF_FLAGS_MASK)
#define REF_FILLER_MASK  (0x00ff000000000000LLU)
#else
#define REF_TYPE_MASK    (0xff00000000000000LLU)
#define REF_ADDRESS_MASK (0x00ffffffffffffffLLU & ~REF_FLAGS_MASK)
#define REF_FILLER_MASK  (0x0000000000000000LLU)
#endif

#define REF_POINTER_MASK (REF_ADDRESS_MASK | REF_FILLER_MASK)
#define REF_META_MASK    (REF_TYPE_MASK | REF_FLAGS_MASK)

INLINE void _ref_set(ResourceRef *ref, void *ptr, uintptr_t type, uintptr_t flags) {
	union { void *ptr; uintptr_t bytes; } cvt;
	cvt.ptr = ptr;
	ref->_internal.handle = (cvt.bytes & REF_POINTER_MASK) | (type << 56) | flags;
}

INLINE void *_ref_get_ptr(ResourceRef *ref) {
	union { void *ptr; uintptr_t bytes; } cvt;
	cvt.bytes = (ref->_internal.handle & REF_POINTER_MASK) | ((ref->_internal.handle & REF_FILLER_MASK) << 8);
	return cvt.ptr;
}

INLINE uintptr_t _ref_get_type(ResourceRef *ref) {
	return ref->_internal.handle >> 56;
}

INLINE uintptr_t _ref_get_flags(ResourceRef *ref) {
	return ref->_internal.handle & REF_FLAGS_MASK;
}

INLINE uintptr_t _ref_test_flags(ResourceRef *ref, uintptr_t flags) {
	assert(flags == (flags & REF_FLAGS_MASK));
	return ref->_internal.handle & flags;
}

#elif defined(RESREF_LAYOUT_PORTABLE)

INLINE void _ref_set(ResourceRef *ref, void *ptr, uintptr_t type, uintptr_t flags) {
	ref->_internal.ptr = ptr;
	ref->_internal.flags = flags;
	ref->_internal.type = type;
}

INLINE void *_ref_get_ptr(ResourceRef *ref) { return ref->_internal.ptr; }
INLINE uint8_t _ref_get_type(ResourceRef *ref) { return ref->_internal.type; }
INLINE uint8_t _ref_get_flags(ResourceRef *ref) { return ref->_internal.flags; }
INLINE uint8_t _ref_test_flags(ResourceRef *ref, uintptr_t flags) { return ref->_internal.flags & flags; }

#endif

/* END Needlessly complicated platform-specific nonsense */

#define verify_flags(flags) assert(flags == (flags & (REF_POINTER_MIN_ALIGNMENT - 1)))
#define verify_ref(ref) assert(res_ref_is_valid(ref))

static ResourceRef create_ref(Resource *res, ResourceFlags flags) {
	verify_flags(flags);

	if(!(flags & RESF_WEAK)) {
		attr_unused int prevrefs = res_inc_refcount(res);
		REF_DEBUG("Ref (%i) %s %s %p [%x]", prevrefs + 1, res_type_idname(res->type), res->name, (void*)res, flags);
	} else {
		// log_debug("Ref (weak) %s %s %p [%x]", res_type_idname(res->type), res->name, (void*)res, flags);
	}

	res->reftime = purge_timer >> 6;

	ResourceRef ref;
	_ref_set(&ref, ASSUME_ALIGNED(res, REF_POINTER_MIN_ALIGNMENT), res->type, flags);

	assert(_ref_get_ptr(&ref) == res);
	assert(_ref_get_type(&ref) == res->type);
	assert(_ref_get_flags(&ref) == flags);

#ifdef DEBUG
	ref._beacon = (flags & RESF_WEAK) ? NULL : malloc(1);
#endif

	verify_ref(ref);
	return ref;
}

static inline Resource *ref_get_resource(ResourceRef ref) {
	verify_ref(ref);

	if(_ref_test_flags(&ref, RESF_ALIEN)) {
		return NULL;
	}

	return _ref_get_ptr(&ref);
}

attr_must_inline
static inline uint_fast8_t ref_get_flags(ResourceRef ref) {
	verify_ref(ref);
	return _ref_get_flags(&ref);
}

static inline void *ref_get_external_data(ResourceRef ref) {
	verify_ref(ref);

	if(_ref_test_flags(&ref, RESF_ALIEN)) {
		return _ref_get_ptr(&ref);
	}

	return NULL;
}

ResourceRef res_ref(ResourceType type, const char *name, ResourceFlags flags) {
	assert(!(flags & RESF_INTERNAL_REF_FLAG_BITS));
	Resource *res = NULL;

	REF_DEBUG("Requested ref for %s %s (%x)", res_type_idname(type), name, flags);
	try_allocate_resource(type, name, flags, &res);
	assume(res != NULL);

	if(!(flags & RESF_LAZY)) {
		REF_DEBUG("Try begin load of %s %s (%x)", res_type_idname(type), name, flags);
		begin_load_resource(res, false);
		REF_DEBUG("Done begin load of %s %s (%x)", res_type_idname(type), name, flags);
	}

	ResourceRef ref = create_ref(res, flags);
	return ref;
}

static ResourceRef ref_copy_internal(ResourceRef ref, ResourceFlags flags) {
	verify_ref(ref);
	Resource *res = ref_get_resource(ref);

	if(res) {
		return create_ref(res, flags);
	} else {
		return res_ref_wrap_external_data(res_ref_type(ref), ref_get_external_data(ref));
	}

	return ref;
}

ResourceRef res_ref_copy(ResourceRef ref) {
	return ref_copy_internal(ref, ref_get_flags(ref) & ~RESF_WEAK);
}

ResourceRef res_ref_copy_weak(ResourceRef ref) {
	return ref_copy_internal(ref, ref_get_flags(ref) | RESF_WEAK);
}

ResourceRef res_ref_wrap_external_data(ResourceType type, void *data) {
	assert((uint)type < RES_NUMTYPES);
	ResourceRef ref;
	_ref_set(&ref, ASSUME_ALIGNED(data, REF_POINTER_MIN_ALIGNMENT), type, RESF_ALIEN);
	// REF_DEBUG("Ref (external) %s %p", res_type_idname(type), data);
	return ref;
}

static void res_unref_one(ResourceRef *ref) {
	verify_ref(*ref);

	if(_ref_test_flags(ref, RESF_WEAK | RESF_ALIEN)) {
		return;
	}

	Resource *res = ref_get_resource(*ref);
	assume(res != NULL);

	attr_unused int prevrc = res_dec_refcount(res);

	REF_DEBUG("UnRef {%p} (%i) %s %s %p", (void*)ref, prevrc - 1, res_type_idname(res_ref_type(*ref)), res_ref_name(*ref), (void*)ref_get_resource(*ref));

#ifdef DEBUG
	free(ref->_beacon);
	ref->_beacon = NULL;
#endif

	_ref_set(ref, NULL, 0, 0);
}

static void res_unref_if_valid_one(ResourceRef *ref) {
	if(res_ref_is_valid(*ref)) {
		res_unref_one(ref);
	} else {
		REF_DEBUG("UnRef {%p} (invalid)", (void*)ref);
	}
}

void res_unref(ResourceRef *ref, size_t numrefs) {
	ResourceRef *end = ref + numrefs;
	for(; ref < end; ++ref) {
		res_unref_one(ref);
	}
}

void res_unref_if_valid(ResourceRef *ref, size_t numrefs) {
	ResourceRef *end = ref + numrefs;
	for(; ref < end; ++ref) {
		res_unref_if_valid_one(ref);
	}
}

void *res_ref_data(ResourceRef ref) {
	Resource *res = ref_get_resource(ref);

	if(res == NULL) {
		void *data = ref_get_external_data(ref);
		assume(data != NULL);
		return data;
	}

	res->reftime = purge_timer >> 6;
	update_flags(res, ref_get_flags(ref));

	switch(res->status) {
		case RES_STATUS_NOT_LOADED:
			// lazy-loaded resource - we need the data right now, so load synchronously
			begin_load_resource(res, true);
			// fallthrough
		case RES_STATUS_LOADING:
			// async load in progress? wait until it's done
			wait_for_resource_load(res);
			assert(res->status == RES_STATUS_LOADED || res->status == RES_STATUS_FAILED);
			// fallthrough
		case RES_STATUS_LOADED:
		case RES_STATUS_FAILED:
			// in the failed case this will be NULL
			return res->data;

		default: UNREACHABLE;
	}

	UNREACHABLE;
}

const char *res_ref_name(ResourceRef ref) {
	Resource *res = ref_get_resource(ref);

	if(res) {
		return res->name;
	}

	return NULL;
}

ResourceType res_ref_type(ResourceRef ref) {
	return _ref_get_type(&ref);
}

ResourceStatus res_ref_status(ResourceRef ref) {
	Resource *res = ref_get_resource(ref);

	if(res != NULL) {
		return res->status;
	}

	return RES_STATUS_EXTERNAL;
}

ResourceStatus res_ref_wait_ready(ResourceRef ref) {
	if(!(ref_get_flags(ref) & RESF_LAZY)) {
		(void)res_ref_data(ref);
	}

	return res_ref_status(ref);
}

res_id_t res_ref_id(ResourceRef ref) {
	return (uintptr_t)_ref_get_ptr(&ref);
}

bool res_refs_are_equivalent(ResourceRef ref1, ResourceRef ref2) {
	if(!res_ref_is_valid(ref1)) {
		return !res_ref_is_valid(ref2);
	}

	if(!res_ref_is_valid(ref2)) {
		return false;
	}

	Resource *res0 = ref_get_resource(ref1);

	if(res0 != NULL) {
		return res0 == ref_get_resource(ref2);
	}

	void *data0 = ref_get_external_data(ref1);
	assert(data0 != NULL);
	return data0 == ref_get_external_data(ref2);
}

bool res_ref_is_valid(ResourceRef ref) {
	return _ref_get_ptr(&ref) != NULL;
}

/* END Refs */

/* BEGIN Legacy */

void *res_get_data(ResourceType type, const char *name, ResourceFlags flags) {
	Resource *res;

	if(try_allocate_resource(type, name, flags, &res)) {
		attr_unused bool is_first = res_inc_refcount_if_zero(res);
		assert(is_first);

		SDL_LockMutex(res->mutex);

		void *data = NULL;
		begin_load_resource(res, true);

		if(res->status != RES_STATUS_FAILED) {
			assert(res->status == RES_STATUS_LOADED);
			assert(res->data != NULL);
			data = res->data;
		}

		res->reftime = purge_timer >> 6;
		SDL_UnlockMutex(res->mutex);
		return data;
	} else {
		res_inc_refcount_if_zero(res);
		update_flags(res, flags);
		begin_load_resource(res, true);
		ResourceStatus status = wait_for_resource_load(res);

		if(status == RES_STATUS_FAILED) {
			return NULL;
		}

		assert(status == RES_STATUS_LOADED);
		assert(res->data != NULL);

		res->reftime = purge_timer >> 6;
		return res->data;
	}
}

void res_group_init(ResourceRefGroup *group, uint init_size) {
	assert(init_size > 0);
	group->num_allocated = init_size;
	group->num_used = 0;
	group->refs = calloc(group->num_allocated, sizeof(*group->refs));
}

void res_group_destroy(ResourceRefGroup *group) {
	if(group == NULL) {
		return;
	}

	if(group->refs) {
		res_unref(group->refs, group->num_used);
		free(group->refs);
	}

	memset(group, 0, sizeof(*group));
}

void res_group_add_ref(ResourceRefGroup *group, ResourceRef ref) {
	assert(group->refs != NULL);
	assert(group->num_allocated > 0);

	if(group->num_allocated == group->num_used) {
		group->num_allocated *= 2;
		group->refs = realloc(group->refs, group->num_allocated * sizeof(*group->refs));
	} else {
		assert(group->num_used < group->num_allocated);
	}

	group->refs[group->num_used++] = ref;
}

void res_group_multi_add(ResourceRefGroup *group, ResourceType type, ResourceFlags flags, const char *firstname, ...) {
	va_list args;
	va_start(args, firstname);

	for(const char *name = firstname; name; name = va_arg(args, const char*)) {
		res_group_add_ref(group, res_ref(type, name, flags));
	}

	va_end(args);
}

bool res_group_is_ready(ResourceRefGroup *group) {
	ResourceRef *end = group->refs + group->num_used;

	for(ResourceRef *ref = group->refs; ref < end; ++ref) {
		if(res_ref_status(*ref) == RES_STATUS_LOADING) {
			return false;
		}
	}

	return true;
}

void res_group_wait_ready(ResourceRefGroup *group) {
	ResourceRef *end = group->refs + group->num_used;

	for(ResourceRef *ref = group->refs; ref < end; ++ref) {
		res_ref_wait_ready(*ref);
	}
}

/* END Legacy */

/* BEGIN Events */

static bool res_event_handler(SDL_Event *event, void *arg) {
	switch(TAISEI_EVENT(event->type)) {
		case TE_RESOURCE_ASYNC_LOADED:
			return resource_asyncload_handler(event, arg);

		case TE_FRAME:
			purge_timer += 1 + (purgatory.count >> 6);
			if(!(purge_timer & 0xff)) {
				res_purge(10, 120);
			}
			return false;

		default: return false;
	}
}

void res_init(void) {
	for(int i = 0; i < RES_NUMTYPES; ++i) {
		ResourceHandler *h = res_type_handler(i);
		alloc_handler(h);

		if(h->procs.init != NULL) {
			h->procs.init();
		}
	}

	events_register_handler(&(EventHandler) {
		.proc = res_event_handler,
		.priority = EPRIO_SYSTEM,
	});
}

static void *preload_shaders(const char *path, void *arg) {
	if(!_handlers[RES_SHADERPROG]->procs.check(path)) {
		return NULL;
	}

	char *name = res_util_basename(RES_PATHPREFIX_SHADERPROG, path);
	res_get_data(RES_SHADERPROG, name, RESF_DEFAULT);
	free(name);

	return NULL;
}

void res_post_init(void) {
	for(uint i = 0; i < RES_NUMTYPES; ++i) {
		ResourceHandler *h = res_type_handler(i);
		assert(h != NULL);

		if(h->procs.post_init) {
			h->procs.post_init();
		}
	}

	if(env_get("TAISEI_PRELOAD_SHADERS", 0)) {
		log_warn("Loading all shaders now due to TAISEI_PRELOAD_SHADERS");
		vfs_dir_walk(RES_PATHPREFIX_SHADERPROG, preload_shaders, NULL);
	}
}

void res_shutdown(void) {
	for(ResourceType type = 0; type < RES_NUMTYPES; ++type) {
		ResourceHandler *handler = res_type_handler(type);
		if(handler->procs.pre_shutdown != NULL) {
			handler->procs.pre_shutdown();
		}
	}

	while(purgatory.list.first) {
		res_purge(0, 0);
	}

	ht_str2ptr_ts_iter_t iter;

	for(ResourceType type = 0; type < RES_NUMTYPES; ++type) {
		ResourceHandler *handler = res_type_handler(type);

		ht_iter_begin(&handler->private.mapping, &iter);
		for(; iter.has_data; ht_iter_next(&iter)) {
			Resource *res = iter.value;

			int refs = SDL_AtomicGet(&res->refcount);
			if(refs > 0) {
				log_warn("%i dangling reference%s to %s `%s`", refs, refs > 1 ? "s" : "", res_type_fullname(res->type), res->name);
			}

			unload_resource(res);
		}
		ht_iter_end(&iter);
	}

	for(ResourceType type = 0; type < RES_NUMTYPES; ++type) {
		ResourceHandler *handler = res_type_handler(type);

		ht_iter_begin(&handler->private.mapping, &iter);
		for(; iter.has_data; ht_iter_next(&iter)) {
			Resource *res = iter.value;
			free_resource(res);
		}
		ht_iter_end(&iter);

		if(handler->procs.shutdown != NULL) {
			handler->procs.shutdown();
		}

		ht_destroy(&handler->private.mapping);
	}

	events_unregister_handler(res_event_handler);
}

void res_purge(uint limit, uint timegap) {
	purgatory_lock();

	if(purgatory.count < 1) {
		assert(purgatory.count >= 0);
		purgatory_unlock();
		return;
	}

	if(purgatory.count < limit || !limit) {
		limit = purgatory.count;
	}

	Resource *parray[limit];
	memset(parray, 0, sizeof(parray));

	int i = 0;
	uint32_t t = purge_timer >> 6;

	while(true) {
		if(t - purgatory.list.first->reftime < timegap) {
			limit = i;
			break;
		}

		parray[i] = purgatory.list.first;
		purgatory_remove(purgatory.list.first);

		if(++i == limit) {
			break;
		}
	}

	assert(i == limit);
	purgatory_check_consistency();
	purgatory_unlock();

	if(limit == 0) {
		return;
	}

	log_debug("Purging %i unreferenced resources (%i left)", limit, purgatory.count);

	for(i = 0; i < limit; ++i) {
		Resource *res = parray[i];
		assume(res != NULL);
		unload_resource(res);
	}

	log_debug("Purged %i unreferenced resources (%i left)", limit, purgatory.count);
}

/* END Events */
