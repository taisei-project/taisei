/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "decompress_wrapper.h"
#include "rwops/rwops_ro.h"
#include "rwops/rwops_zstd.h"
#include "util/strbuf.h"

// NOTE: Largely based on readonly_wrapper. Sorry for the copypasta.
// This is currently hardcoded to only support transparent decompression of .zst files.

#define ZST_SUFFIX ".zst"
#define WRAPPED(n) ((VFSNode*)(n)->data1)

struct decomp_data {
	uint compr_zstd : 1;
};

static_assert_nomsg(sizeof(struct decomp_data) <= sizeof(void*));

static char *vfs_decomp_repr(VFSNode *node) {
	char *wrapped_repr = vfs_node_repr(WRAPPED(node), false);
	char *repr = strjoin("decompress view of ", wrapped_repr, NULL);
	mem_free(wrapped_repr);
	return repr;
}

static void vfs_decomp_free(VFSNode *node) {
	vfs_decref(WRAPPED(node));
}

static VFSInfo vfs_decomp_query(VFSNode *node) {
	VFSInfo info = vfs_node_query(WRAPPED(node));
	info.is_readonly = true;
	return info;
}

static char *vfs_decomp_syspath(VFSNode *node) {
	return vfs_node_syspath(WRAPPED(node));
}

static bool vfs_decomp_mount(VFSNode *mountroot, const char *subname, VFSNode *mountee) {
	vfs_set_error("Read-only filesystem");
	return false;
}

static bool vfs_decomp_unmount(VFSNode *mountroot, const char *subname) {
	vfs_set_error("Read-only filesystem");
	return false;
}

static bool vfs_decomp_mkdir(VFSNode *parent, const char *subdir) {
	vfs_set_error("Read-only filesystem");
	return false;
}

static VFSNode *vfs_decomp_locate(VFSNode *dirnode, const char *path) {
	VFSNode *wrapped = WRAPPED(dirnode);
	VFSNode *child = vfs_node_locate(wrapped, path);
	struct decomp_data data = { 0 };

	VFSNode *ochild = NULL;

	if(child != NULL && !vfs_node_query(child).exists) {
		ochild = child;
		child = NULL;
	}

	if(child == NULL) {
		size_t plen = strlen(path);
		char p[plen + sizeof(ZST_SUFFIX)];
		memcpy(p, path, plen);
		memcpy(p + plen, ZST_SUFFIX, sizeof(ZST_SUFFIX));

		child = vfs_node_locate(wrapped, p);

		if(child == NULL) {
			child = ochild;
		} else {
			VFSInfo i = vfs_node_query(child);

			if(i.error || i.is_dir) {
				vfs_decref(child);
				child = ochild;
			} else {
				data.compr_zstd = true;

				if(ochild) {
					vfs_decref(ochild);
					ochild = NULL;
				}
			}
		}
	}

	if(child == NULL) {
		return NULL;
	}

	VFSNode *wrapped_child = NOT_NULL(vfs_decomp_wrap(child));
	vfs_decref(child);
	memcpy(&wrapped_child->data2, &data, sizeof(data));

	return wrapped_child;
}

static SDL_RWops *vfs_decomp_open(VFSNode *filenode, VFSOpenMode mode) {
	if(mode & VFS_MODE_WRITE) {
		vfs_set_error("Read-only filesystem");
		return NULL;
	}

	struct decomp_data data;
	memcpy(&data, &filenode->data2, sizeof(data));

	SDL_RWops *raw = vfs_node_open(WRAPPED(filenode), mode);

	if(!raw) {
		return NULL;
	}

	if(data.compr_zstd) {
		return SDL_RWWrapZstdReaderSeekable(raw, -1, true);
	}

	return SDL_RWWrapReadOnly(raw, true);
}

struct decomp_iter_data {
	ht_str2int_t visited;
	void *opaque;
	StringBuffer temp_buf;
	bool clear_buffer;
};

static const char *_vfs_decomp_iter(VFSNode *wrapped, struct decomp_iter_data *i) {
	if(i->temp_buf.buf_size > 0 && i->temp_buf.start[0]) {
		if(i->clear_buffer) {
			i->temp_buf.start[0] = 0;
			strbuf_clear(&i->temp_buf);
		} else {
			i->clear_buffer = true;
			return i->temp_buf.start;
		}
	}

	const char *r = vfs_node_iter(wrapped, &i->opaque);

	if(!r) {
		vfs_node_iter_stop(wrapped, &i->opaque);
		i->opaque = NULL;
		return NULL;
	}

	if(strendswith(r, ZST_SUFFIX)) {
		strbuf_ncat(&i->temp_buf, strlen(r) - sizeof(ZST_SUFFIX) + 1, r);
	}

	return r;
}

static const char *vfs_decomp_iter(VFSNode *node, void **opaque) {
	VFSNode *wrapped = WRAPPED(node);
	struct decomp_iter_data *i = *opaque;

	if(!i) {
		i = ALLOC(typeof(*i));
		ht_create(&i->visited);
		*opaque = i;
	}

	for(;;) {
		const char *r = _vfs_decomp_iter(wrapped, i);

		if(!r) {
			return NULL;
		}

		if(!ht_get(&i->visited, r, false)) {
			ht_set(&i->visited, r, true);
			return r;
		}
	}
}

static void vfs_decomp_iter_stop(VFSNode *node, void **opaque) {
	VFSNode *wrapped = WRAPPED(node);
	struct decomp_iter_data *i = *opaque;

	if(i) {
		vfs_node_iter_stop(wrapped, &i->opaque);
		ht_destroy(&i->visited);
		strbuf_free(&i->temp_buf);
		mem_free(i);
		*opaque = NULL;
	}
}

static VFSNodeFuncs vfs_funcs_decomp = {
	.repr = vfs_decomp_repr,
	.query = vfs_decomp_query,
	.free = vfs_decomp_free,
	.locate = vfs_decomp_locate,
	.syspath = vfs_decomp_syspath,
	.iter = vfs_decomp_iter,
	.iter_stop = vfs_decomp_iter_stop,
	.mkdir = vfs_decomp_mkdir,
	.open = vfs_decomp_open,
	.mount = vfs_decomp_mount,
	.unmount = vfs_decomp_unmount,
};

VFSNode *vfs_decomp_wrap(VFSNode *base) {
	if(base == NULL) {
		return NULL;
	}

	vfs_incref(base);

	VFSNode *wrapper = vfs_alloc();
	wrapper->funcs = &vfs_funcs_decomp;
	wrapper->data1 = base;
	return wrapper;
}

bool vfs_make_decompress_view(const char *path) {
	char buf[strlen(path)+1], *path_parent, *path_subdir;
	vfs_path_normalize(path, buf);

	char npath[strlen(path)+1];
	strcpy(npath, buf);

	vfs_path_split_right(buf, &path_parent, &path_subdir);

	VFSNode *parent = vfs_locate(vfs_root, path_parent);

	if(parent == NULL) {
		return false;
	}

	VFSNode *node = vfs_locate(parent, path_subdir);

	if(node == NULL) {
		vfs_decref(parent);
		return false;
	}

	if(node->funcs == &vfs_funcs_decomp) {
		vfs_decref(node);
		vfs_decref(parent);
		return true;
	}

	if(!vfs_node_unmount(parent, path_subdir)) {
		vfs_decref(node);
		vfs_decref(parent);
		return false;
	}

	VFSNode *wrapper = vfs_decomp_wrap(node);
	assert(wrapper != NULL);
	vfs_decref(node);

	if(!vfs_node_mount(parent, path_subdir, wrapper)) {
		log_fatal("Couldn't remount '%s' - VFS left in inconsistent state! Error: %s", npath, vfs_get_error());
		UNREACHABLE;
	}

	vfs_decref(parent);
	assert(vfs_query(path).is_readonly);
	return true;
}
