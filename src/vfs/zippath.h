/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef TAISEI_VFS_ZIPPATH
#define TAISEI_VFS_ZIPPATH

#include <zip.h>

#include "private.h"
#include "zipfile.h"

typedef struct VFSZipPathData {
    VFSNode *zipnode;
    VFSZipFileTLS *tls;
    uint64_t index;
    VFSInfo info;
} VFSZipPathData;

void vfs_zippath_init(VFSNode *node, VFSNode *zipnode, VFSZipFileTLS *tls, zip_int64_t idx);

#endif
