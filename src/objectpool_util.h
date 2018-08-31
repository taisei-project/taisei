/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "objectpool.h"

bool objpool_is_full(ObjectPool *pool);
size_t objpool_object_contents_size(ObjectPool *pool);
void *objpool_object_contents(ObjectPool *pool, ObjectInterface *obj, size_t *out_size);
