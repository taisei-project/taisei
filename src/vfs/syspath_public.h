/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_vfs_syspath_public_h
#define IGUARD_vfs_syspath_public_h

#include "taisei.h"

enum {
	VFS_SYSPATH_MOUNT_READONLY = (1 << 0),
	VFS_SYSPATH_MOUNT_MKDIR = (1 << 1),
	VFS_SYSPATH_MOUNT_DECOMPRESSVIEW = (1 << 2),
};

bool vfs_mount_syspath(const char *mountpoint, const char *fspath, uint flags)
	attr_nonnull(1, 2) attr_nodiscard;

char vfs_get_syspath_separator(void);
void vfs_syspath_normalize(char *buf, size_t bufsize, const char *path);
char *vfs_syspath_normalize_inplace(char *path);
void vfs_syspath_join(char *buf, size_t bufsize, const char *parent, const char *child);
char *vfs_syspath_join_alloc(const char *parent, const char *child);

#endif // IGUARD_vfs_syspath_public_h
