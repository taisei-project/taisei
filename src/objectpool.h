/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_objectpool_h
#define IGUARD_objectpool_h

#include "taisei.h"

#include <stdlib.h>

#include "list.h"

#ifdef DEBUG
	#define OBJPOOL_DEBUG
#endif

#ifdef OBJPOOL_DEBUG
	#define IF_OBJPOOL_DEBUG(code) code
#else
	#define IF_OBJPOOL_DEBUG(code)
#endif

typedef struct ObjectPool ObjectPool;
typedef struct ObjectInterface ObjectInterface;
typedef struct ObjectPoolStats ObjectPoolStats;

struct ObjectPoolStats {
	const char *tag;
	size_t capacity;
	size_t usage;
	size_t peak_usage;
};

#define OBJECT_INTERFACE_BASE(typename) struct { \
	LIST_INTERFACE(typename); \
	IF_OBJPOOL_DEBUG( \
		struct { \
			bool used; \
		} _object_private; \
	) \
}

#define OBJECT_INTERFACE(typename) union { \
	ObjectInterface object_interface; \
	OBJECT_INTERFACE_BASE(typename); \
}

struct ObjectInterface {
	OBJECT_INTERFACE_BASE(ObjectInterface);
};

#ifdef USE_GNU_EXTENSIONS
	// Do some compile-time checks to make sure the type is compatible with object pools

	#define OBJPOOL_ALLOC(typename,max_objects) (__extension__ ({ \
		static_assert(__builtin_types_compatible_p(ObjectInterface, __typeof__(((typename*)(0))->object_interface)), \
			#typename " must implement ObjectInterface (use the OBJECT_INTERFACE macro)"); \
		static_assert(__builtin_offsetof(typename, object_interface) == 0, \
			"object_interface must be the first member in " #typename); \
		objpool_alloc(sizeof(typename), max_objects, #typename); \
	}))
#else
	#define OBJPOOL_ALLOC(typename,max_objects) objpool_alloc(sizeof(typename), max_objects, #typename)
#endif

ObjectPool *objpool_alloc(size_t obj_size, size_t max_objects, const char *tag);
void objpool_free(ObjectPool *pool);
ObjectInterface *objpool_acquire(ObjectPool *pool);
void objpool_release(ObjectPool *pool, ObjectInterface *object);
void objpool_get_stats(ObjectPool *pool, ObjectPoolStats *stats);
void objpool_memtest(ObjectPool *pool, ObjectInterface *object);
size_t objpool_object_size(ObjectPool *pool);

#endif // IGUARD_objectpool_h
