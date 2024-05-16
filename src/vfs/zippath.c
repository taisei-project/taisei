/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "zipfile.h"
#include "zipfile_impl.h"
#include "syspath.h"
#include "rwops/all.h"

#define ZTLS(zpnode) vfs_zipfile_get_tls((zpnode)->zipnode, true)

static const char *vfs_zippath_name(VFSZipPathNode *zpnode) {
	return zip_get_name(ZTLS(zpnode)->zip, zpnode->index, 0);
}

static void vfs_zippath_free(VFSNode *node) {
	auto zpnode = VFS_NODE_CAST(VFSZipPathNode, node);
	vfs_decref(zpnode->zipnode);
}

static char *vfs_zippath_repr(VFSNode *node) {
	auto zpnode = VFS_NODE_CAST(VFSZipPathNode, node);
	char *ziprepr = vfs_node_repr(zpnode->zipnode, false);
	char *zpathrepr = strfmt("%s '%s' in %s",
		zpnode->info.is_dir ? "directory" : "file", vfs_zippath_name(zpnode), ziprepr);
	mem_free(ziprepr);
	return zpathrepr;
}

static char *vfs_zippath_syspath(VFSNode *node) {
	auto zpnode = VFS_NODE_CAST(VFSZipPathNode, node);
	char *zippath = vfs_node_repr(zpnode->zipnode, true);
	char *subpath = strfmt("%s%c%s", zippath, vfs_syspath_separators[0], vfs_zippath_name(zpnode));
	mem_free(zippath);
	vfs_syspath_normalize_inplace(subpath);
	return subpath;
}

static VFSInfo vfs_zippath_query(VFSNode *node) {
	return VFS_NODE_CAST(VFSZipPathNode, node)->info;
}

static VFSNode *vfs_zippath_locate(VFSNode *node, const char *path) {
	auto zpnode = VFS_NODE_CAST(VFSZipPathNode, node);
	const char *mypath = vfs_zippath_name(zpnode);
	char fullpath[strlen(mypath) + strlen(path) + 2];
	snprintf(fullpath, sizeof(fullpath), "%s%c%s", mypath, VFS_PATH_SEPARATOR, path);
	vfs_path_normalize_inplace(fullpath);

	return vfs_locate(zpnode->zipnode, fullpath);
}

static const char *vfs_zippath_iter(VFSNode *node, void **opaque) {
	auto zpnode = VFS_NODE_CAST(VFSZipPathNode, node);
	VFSZipFileIterData *idata = *opaque;

	if(!zpnode->info.is_dir) {
		return NULL;
	}

	if(!idata) {
		idata = ALLOC(typeof(*idata));
		idata->num = zip_get_num_entries(ZTLS(zpnode)->zip, 0);
		idata->idx = zpnode->index;
		idata->prefix = vfs_zippath_name(zpnode);
		idata->prefix_len = strlen(idata->prefix);
		*opaque = idata;
	}

	return vfs_zipfile_iter_shared(idata, ZTLS(zpnode));
}

#define vfs_zippath_iter_stop vfs_zipfile_iter_stop

static SDL_RWops *vfs_zippath_open(VFSNode *node, VFSOpenMode mode) {
	auto zpnode = VFS_NODE_CAST(VFSZipPathNode, node);

	if(mode & VFS_MODE_WRITE) {
		vfs_set_error("ZIP archives are read-only");
		return NULL;
	}

	if(mode & VFS_MODE_SEEKABLE && zpnode->compression != ZIP_CM_STORE) {
		char *repr = vfs_node_repr(node, true);
		log_warn("Opening compressed file '%s' in seekable mode, this is suboptimal. Consider storing this file without compression", repr);
		mem_free(repr);
	}

	return vfs_zippath_make_rwops(zpnode);
}

VFS_NODE_FUNCS(VFSZipPathNode, {
	.repr = vfs_zippath_repr,
	.query = vfs_zippath_query,
	.free = vfs_zippath_free,
	.syspath = vfs_zippath_syspath,
	//.mount = vfs_zippath_mount,
	.locate = vfs_zippath_locate,
	.iter = vfs_zippath_iter,
	.iter_stop = vfs_zippath_iter_stop,
	//.mkdir = vfs_zippath_mkdir,
	.open = vfs_zippath_open,
});

VFSNode *vfs_zippath_create(VFSZipNode *zipnode, zip_int64_t idx) {
	auto zpnode = VFS_ALLOC(VFSZipPathNode, {
		.zipnode = zipnode,
		.index = idx,
		.size = -1,
		.compressed_size = -1,
		.compression = ZIP_CM_STORE,
		.info = {
			.exists = true,
			.is_readonly = true,
		},
	});

	zpnode->info.is_dir = ('/' == *(strchr(vfs_zippath_name(zpnode), 0) - 1));

	zip_stat_t zstat;

	if(zip_stat_index(ZTLS(zpnode)->zip, zpnode->index, 0, &zstat) < 0) {
		log_warn("zip_stat_index(%"PRIi64") failed: %s", idx, zip_error_strerror(&ZTLS(zpnode)->error));
	} else {
		if(zstat.valid & ZIP_STAT_SIZE) {
			zpnode->size = zstat.size;
		}

		if(zstat.valid & ZIP_STAT_COMP_SIZE) {
			zpnode->compressed_size = zstat.comp_size;
		}

		if(zstat.valid & ZIP_STAT_COMP_METHOD) {
			zpnode->compression = zstat.comp_method;
		}
	}

	vfs_incref(zipnode);
	return &zpnode->as_generic;
}
