/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "private.h"

#include "../zipfile.h"

#define ZIP_NAME_MAXLEN 256

/* zipfile */

VFS_NODE_TYPE(VFSZipNode, {
	MemArena arena;
	ZipContext ctx;
	VFSNode *source;
	VFSMMapTicket mmap_ticket;
	ZipEntry *entries;
	uint32_t num_entries;
	uint32_t max_name_len;
});

typedef struct VFSZipFileIterData {
	const ZipEntry *entry;
	const char *prefix;
	uint32_t prefix_len;
	uint32_t last_name_len;
	char name_buf[];
} VFSZipFileIterData;

const char *vfs_zipfile_iter_shared(const VFSZipNode *znode, VFSZipFileIterData *idata);
void vfs_zipfile_iter_stop(VFSNode *node, void **opaque);

/* zippath */

VFS_NODE_TYPE(VFSZipPathNode, {
	VFSZipNode *znode;
	const ZipEntry *entry;
});

VFSNode *vfs_zippath_create(VFSZipNode *zipnode, const ZipEntry *entry);
