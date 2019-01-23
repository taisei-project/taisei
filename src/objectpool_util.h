/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_objectpool_util_h
#define IGUARD_objectpool_util_h

#include "taisei.h"

#include "objectpool.h"

bool objpool_is_full(ObjectPool *pool);
size_t objpool_object_contents_size(ObjectPool *pool);
void *objpool_object_contents(ObjectPool *pool, ObjectInterface *obj, size_t *out_size);

#endif // IGUARD_objectpool_util_h
