/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_vfs_readonly_wrapper_h
#define IGUARD_vfs_readonly_wrapper_h

#include "taisei.h"

#include "readonly_wrapper_public.h"
#include "private.h"

VFSNode* vfs_ro_wrap(VFSNode *base);

#endif // IGUARD_vfs_readonly_wrapper_h
