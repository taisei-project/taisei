/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_vfs_error_h
#define IGUARD_vfs_error_h

#include "taisei.h"

void vfs_set_error(char *fmt, ...) attr_printf(1, 2);
void vfs_set_error_from_sdl(void);

#endif // IGUARD_vfs_error_h
