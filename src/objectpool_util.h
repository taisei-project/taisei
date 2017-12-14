
#pragma once

#include "objectpool.h"

void objpool_release_list(ObjectPool *pool, List **dest);
bool objpool_is_full(ObjectPool *pool);
