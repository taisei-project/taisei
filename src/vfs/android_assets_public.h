/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_vfs_android_assets_public_h
#define IGUARD_vfs_android_assets_public_h

#include "taisei.h"

bool vfs_mount_assets(const char *mountpoint, const char *apath)
	attr_nonnull(1, 2) attr_nodiscard;

#endif // IGUARD_vfs_android_assets_public_h
