/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef TAISEI_VFS_SYSPATH
#define TAISEI_VFS_SYSPATH

#include "private.h"

extern char vfs_syspath_prefered_separator;
bool vfs_syspath_init(VFSNode *node, const char *path);
void vfs_syspath_normalize(char *buf, size_t bufsize, const char *path);

#endif
