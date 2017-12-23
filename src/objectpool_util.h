/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <stdint.h>

#include "objectpool.h"

void objpool_release_list(ObjectPool *pool, List **dest);
bool objpool_is_full(ObjectPool *pool);
size_t objpool_object_contents_size(ObjectPool *pool);
void *objpool_object_contents(ObjectPool *pool, ObjectInterface *obj, size_t *out_size);
