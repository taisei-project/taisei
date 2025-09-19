/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "private.h"

typedef struct VFSResIndexFSContext VFSResIndexFSContext;

typedef SDL_IOStream *(*VFSResIndexFSOpenProc)(VFSResIndexFSContext *ctx, const char *content_id, VFSOpenMode mode);
typedef void (*VFSResIndexFSFreeProc)(VFSResIndexFSContext *ctx);

typedef struct VFSResIndexFSContext {
	struct {
		VFSResIndexFSOpenProc open;
		VFSResIndexFSFreeProc free;
	} procs;
	void *userdata;
} VFSResIndexFSContext;

VFSNode *vfs_resindex_create(VFSResIndexFSContext *ctx);

typedef struct RIdxDirEntry {
	const char *name;
	int subdirs_ofs;
	int subdirs_num;
	int files_ofs;
	int files_num;
} RIdxDirEntry;

typedef struct RIdxFileEntry {
	const char *name;
	const char *content_id;
} RIdxFileEntry;

uint resindex_num_dir_entries(void);
const RIdxDirEntry *resindex_get_dir_entry(uint idx);

uint resindex_num_file_entries(void);
const RIdxFileEntry *resindex_get_file_entry(uint idx);
