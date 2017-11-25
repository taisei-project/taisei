/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "zipfile.h"

bool vfs_zipfile_init(VFSNode *node, VFSNode *source) {
    vfs_set_error("Compiled without ZIP support");
    return false;
}
