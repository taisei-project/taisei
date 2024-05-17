/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

// #define VFS_LOG_ERRORS

void vfs_set_error(char *fmt, ...) attr_printf(1, 2);
void vfs_set_error_from_sdl(void);

#ifdef VFS_LOG_ERRORS
#include "log.h"
#define vfs_set_error(fmt, ...) ({ \
	log_debug("vfs_set_error: " fmt, ##__VA_ARGS__); \
	vfs_set_error(fmt, ##__VA_ARGS__); \
})
#endif
