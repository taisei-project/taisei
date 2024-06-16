/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <SDL_platform.h>

#define VFS_PATH_SEPARATOR_STR "/"
#define VFS_PATH_SEPARATOR VFS_PATH_SEPARATOR_STR[0]
#define VFS_IS_PATH_SEPARATOR(chr) ((chr) == VFS_PATH_SEPARATOR)

static_assert(sizeof(VFS_PATH_SEPARATOR_STR) == 2, "No more than one VFS path separator, please");

char *vfs_path_normalize(const char *path, char *out);
char *vfs_path_normalize_alloc(const char *path);
char *vfs_path_normalize_inplace(char *path);
void vfs_path_split_left(char *path, char **lpath, char **rpath);
void vfs_path_split_right(char *path, char **lpath, char **rpath);
void vfs_path_root_prefix(char *path);
void vfs_path_resolve_relative(char *buf, size_t bufsize, const char *basepath, const char *relpath); // NOTE: doesn't normalize
