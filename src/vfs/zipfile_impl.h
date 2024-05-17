/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

// libzip on clang creates useless noise
DIAGNOSTIC_CLANG(push)
DIAGNOSTIC_CLANG(ignored "-Wnullability-extension")
DIAGNOSTIC_CLANG(ignored "-Wnullability-completeness")
#include <zip.h>
DIAGNOSTIC_CLANG(pop)

#include "util/libzip_compat.h"   // IWYU pragma: export

#include "private.h"
#include "hashtable.h"

/* zipfile */

VFS_NODE_TYPE(VFSZipNode, {
	VFSNode *source;
	ht_str2int_t pathmap;
	SDL_TLSID tls_id;
});

typedef struct VFSZipFileTLS {
	zip_t *zip;
	SDL_RWops *stream;
	zip_error_t error;
} VFSZipFileTLS;

typedef struct VFSZipFileIterData {
	zip_int64_t idx;
	zip_int64_t num;
	const char *prefix;
	size_t prefix_len;
	char *allocated;
} VFSZipFileIterData;

const char *vfs_zipfile_iter_shared(VFSZipFileIterData *idata, VFSZipFileTLS *tls);
void vfs_zipfile_iter_stop(VFSNode *node, void **opaque);
VFSZipFileTLS *vfs_zipfile_get_tls(VFSZipNode *znode, bool create);

/* zippath */

VFS_NODE_TYPE(VFSZipPathNode, {
	VFSZipNode *zipnode;
	uint64_t index;
	ssize_t size;
	ssize_t compressed_size;
	VFSInfo info;
	uint16_t compression;
});

VFSNode *vfs_zippath_create(VFSZipNode *zipnode, zip_int64_t idx);
SDL_RWops *vfs_zippath_make_rwops(VFSZipPathNode *zpnode) attr_nonnull_all;
