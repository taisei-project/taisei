/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_vfs_zipfile_impl_h
#define IGUARD_vfs_zipfile_impl_h

#include "taisei.h"

// libzip on clang creates useless noise
DIAGNOSTIC_CLANG(push)
DIAGNOSTIC_CLANG(ignored "-Wnullability-extension")
#include <zip.h>
DIAGNOSTIC_CLANG(pop)

#ifndef ZIP_CM_ZSTD
	#define ZIP_CM_ZSTD 93
#endif

#include "private.h"
#include "hashtable.h"

/* zipfile */

typedef struct VFSZipFileTLS {
	zip_t *zip;
	SDL_RWops *stream;
	zip_error_t error;
} VFSZipFileTLS;

typedef struct VFSZipFileData {
	VFSNode *source;
	ht_str2int_t pathmap;
	SDL_TLSID tls_id;
} VFSZipFileData;

typedef struct VFSZipFileIterData {
	zip_int64_t idx;
	zip_int64_t num;
	const char *prefix;
	size_t prefix_len;
	char *allocated;
} VFSZipFileIterData;

const char* vfs_zipfile_iter_shared(VFSNode *node, VFSZipFileData *zdata, VFSZipFileIterData *idata, VFSZipFileTLS *tls);
void vfs_zipfile_iter_stop(VFSNode *node, void **opaque);

/* zippath */

typedef struct VFSZipPathData {
	VFSNode *zipnode;
	uint64_t index;
	ssize_t size;
	ssize_t compressed_size;
	VFSInfo info;
	uint16_t compression;
} VFSZipPathData;

void vfs_zippath_init(VFSNode *node, VFSNode *zipnode, zip_int64_t idx);
VFSZipFileTLS* vfs_zipfile_get_tls(VFSNode *node, bool create);

#endif // IGUARD_vfs_zipfile_impl_h
