/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "zipfile.h"
#include "zipfile_impl.h"
#include "syspath.h"
#include "rwops/all.h"

#define ZTLS(pdata) vfs_zipfile_get_tls((pdata)->zipnode, true)

static const char* vfs_zippath_name(VFSNode *node) {
	VFSZipPathData *zdata = node->data1;
	return zip_get_name(ZTLS(zdata)->zip, zdata->index, 0);
}

static void vfs_zippath_free(VFSNode *node) {
	free(node->data1);
}

static char* vfs_zippath_repr(VFSNode *node) {
	VFSZipPathData *zdata = node->data1;
	char *ziprepr = vfs_node_repr(zdata->zipnode, false);
	char *zpathrepr = strfmt("%s '%s' in %s",
		zdata->info.is_dir ? "directory" : "file", vfs_zippath_name(node), ziprepr);
	free(ziprepr);
	return zpathrepr;
}

static char* vfs_zippath_syspath(VFSNode *node) {
	VFSZipPathData *zdata = node->data1;
	char *zippath = vfs_node_repr(zdata->zipnode, true);
	char *subpath = strfmt("%s%c%s", zippath, vfs_syspath_separators[0], vfs_zippath_name(node));
	free(zippath);
	vfs_syspath_normalize_inplace(subpath);
	return subpath;
}

static VFSInfo vfs_zippath_query(VFSNode *node) {
	VFSZipPathData *zdata = node->data1;
	return zdata->info;
}

static VFSNode* vfs_zippath_locate(VFSNode *node, const char *path) {
	VFSZipPathData *zdata = node->data1;

	const char *mypath = vfs_zippath_name(node);
	char fullpath[strlen(mypath) + strlen(path) + 2];
	snprintf(fullpath, sizeof(fullpath), "%s%c%s", mypath, VFS_PATH_SEPARATOR, path);
	vfs_path_normalize_inplace(fullpath);

	return vfs_locate(zdata->zipnode, fullpath);
}

static const char* vfs_zippath_iter(VFSNode *node, void **opaque) {
	VFSZipPathData *zdata = node->data1;
	VFSZipFileIterData *idata = *opaque;

	if(!zdata->info.is_dir) {
		return NULL;
	}

	if(!idata) {
		idata = calloc(1, sizeof(VFSZipFileIterData));
		idata->num = zip_get_num_entries(ZTLS(zdata)->zip, 0);
		idata->idx = zdata->index;
		idata->prefix = vfs_zippath_name(node);
		idata->prefix_len = strlen(idata->prefix);
		*opaque = idata;
	}

	return vfs_zipfile_iter_shared(node, zdata->zipnode->data1, idata, ZTLS(zdata));
}

#define vfs_zippath_iter_stop vfs_zipfile_iter_stop

static SDL_RWops* vfs_zippath_open(VFSNode *node, VFSOpenMode mode) {
	if(mode & VFS_MODE_WRITE) {
		vfs_set_error("ZIP archives are read-only");
		return NULL;
	}

	VFSZipPathData *zdata = node->data1;

	if(mode & VFS_MODE_SEEKABLE && zdata->compression != ZIP_CM_STORE) {
		char *repr = vfs_node_repr(node, true);
		log_warn("Opening compressed file '%s' in seekable mode, this is suboptimal. Consider storing this file without compression", repr);
		free(repr);
	}

	return SDL_RWFromZipFile(node, zdata);
}

static VFSNodeFuncs vfs_funcs_zippath = {
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
};

void vfs_zippath_init(VFSNode *node, VFSNode *zipnode, zip_int64_t idx) {
	VFSZipPathData *zdata = calloc(1, sizeof(VFSZipPathData));
	zdata->zipnode = zipnode;
	zdata->index = idx;
	zdata->size = -1;
	zdata->compression = ZIP_CM_STORE;
	node->data1 = zdata;

	zdata->info.exists = true;
	zdata->info.is_readonly = true;

	if('/' == *(strchr(vfs_zippath_name(node), 0) - 1)) {
		zdata->info.is_dir = true;
	}

	zip_stat_t zstat;

	if(zip_stat_index(ZTLS(zdata)->zip, zdata->index, 0, &zstat) < 0) {
		log_warn("zip_stat_index(%"PRIi64") failed: %s", idx, zip_error_strerror(&ZTLS(zdata)->error));
	} else {
		if(zstat.valid & ZIP_STAT_SIZE) {
			zdata->size = zstat.size;
		}

		if(zstat.valid & ZIP_STAT_COMP_METHOD) {
			zdata->compression = zstat.comp_method;
		}
	}

	node->funcs = &vfs_funcs_zippath;

	const char *path = vfs_zippath_name(node);
	char buf[strlen(path)+1], *base, *name;
	strcpy(buf, path);
	vfs_path_split_right(buf, &base, &name);
}
