/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "resindex.h"
#include "resindex_layered.h"
#include "resindex_layered_public.h"

static SDL_IOStream *vfs_resindex_layered_open(VFSResIndexFSContext *ctx, const char *content_id, VFSOpenMode mode) {
	const char *backend_path = ctx->userdata;
	size_t pathlen = strlen(backend_path) + 1 + strlen(content_id);
	char path[pathlen + 1];
	snprintf(path, sizeof(path), "%s" VFS_PATH_SEPARATOR_STR "%s", backend_path, content_id);
	return vfs_open(path, mode);
}

static void vfs_resindex_layered_free(VFSResIndexFSContext *ctx) {
	mem_free(ctx->userdata);
	mem_free(ctx);
}

VFSNode *vfs_resindex_layered_create(const char *backend_vfspath) {
	auto ctx = ALLOC(VFSResIndexFSContext);
	ctx->procs.open = vfs_resindex_layered_open;
	ctx->procs.free = vfs_resindex_layered_free;
	ctx->userdata = vfs_path_normalize_alloc(backend_vfspath);
	return vfs_resindex_create(ctx);
}

bool vfs_mount_resindex_layered(const char *mountpoint, const char *backend_vfspath) {
	return vfs_mount_or_decref(vfs_root, mountpoint, vfs_resindex_layered_create(backend_vfspath));
}
