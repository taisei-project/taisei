/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "resindex.h"

VFS_NODE_TYPE(VFSResIndexNode, {
	void *index_entry;
	VFSResIndexFSContext *context;
	struct VFSResIndexNode *root_node;
});

static const RIdxDirEntry ridx_dirs[] = {
	#define DIR(_idx, _name, _subdirs_ofs, _subdirs_num, _files_ofs, _files_num) \
		[_idx] = { _name, _subdirs_ofs, _subdirs_num, _files_ofs, _files_num },
	#define FILE(...)
	#include "res-index.inc.h"
	#undef DIR
	#undef FILE
};

static const RIdxFileEntry ridx_files[] = {
	#define DIR(...)
	#define FILE(_idx, _id, _name, _srcpath) \
		[_idx] = { _name, _id },
	#include "res-index.inc.h"
	#undef DIR
	#undef FILE
};

#define RIDX_IS_DIR(p) \
	((RIdxDirEntry*)(p) >= ridx_dirs && (RIdxDirEntry*)(p) <= ridx_dirs + ARRAY_SIZE(ridx_dirs))

#define RIDX_IS_FILE(p) \
	((RIdxFileEntry*)(p) >= ridx_files && (RIdxFileEntry*)(p) <= ridx_files + ARRAY_SIZE(ridx_files))

#define RIDX_AS_DIR(p) ({ \
	assert(RIDX_IS_DIR(p)); \
	CASTPTR_ASSUME_ALIGNED(p, RIdxDirEntry); \
})

#define RIDX_AS_FILE(p) ({ \
	assert(RIDX_IS_FILE(p)); \
	CASTPTR_ASSUME_ALIGNED(p, RIdxFileEntry); \
})

uint resindex_num_dir_entries(void) {
	return ARRAY_SIZE(ridx_dirs);
}

const RIdxDirEntry *resindex_get_dir_entry(uint idx) {
	assert(idx < ARRAY_SIZE(ridx_dirs));
	return ridx_dirs + idx;
}

uint resindex_num_file_entries(void) {
	return ARRAY_SIZE(ridx_files);
}

const RIdxFileEntry *resindex_get_file_entry(uint idx) {
	assert(idx < ARRAY_SIZE(ridx_files));
	return ridx_files + idx;
}

INLINE const char *ridx_dir_name(const RIdxDirEntry *d, const char *rootname) {
	if(d == &ridx_dirs[0]) {
		assert(d->name == NULL);
		return rootname;
	}

	return NOT_NULL(d->name);
}

attr_unused
INLINE const char *ridx_node_name(VFSResIndexNode *rinode) {
	if(RIDX_IS_DIR(rinode->index_entry)) {
		return ridx_dir_name(RIDX_AS_DIR(rinode->index_entry), "<root>");
	}
	return RIDX_AS_FILE(rinode->index_entry)->name;
}

INLINE bool ridx_node_is_root(VFSResIndexNode *rinode) {
	assert(rinode->root_node == rinode);
	return rinode->index_entry == &ridx_dirs[0];
}

static const RIdxDirEntry *ridx_subdir_lookup(const RIdxDirEntry *parent, const char *name) {
	const RIdxDirEntry *begin = ridx_dirs + parent->subdirs_ofs;
	const RIdxDirEntry *end = begin + parent->subdirs_num;

	for(const RIdxDirEntry *d = begin; d < end; ++d) {
		if(!strcmp(name, NOT_NULL(d->name))) {
			return d;
		}
	}

	return NULL;
}

static const RIdxFileEntry *ridx_file_lookup(const RIdxDirEntry *parent, const char *name) {
	const RIdxFileEntry *begin = ridx_files + parent->files_ofs;
	const RIdxFileEntry *end = begin + parent->files_num;

	for(const RIdxFileEntry *f = begin; f < end; ++f) {
		if(!strcmp(name, f->name)) {
			return f;
		}
	}

	return NULL;
}

static VFSResIndexNode *ridx_alloc_node(VFSResIndexNode *parent, void *content);

static VFSInfo vfs_resindex_query(VFSNode *node) {
	return (VFSInfo) {
		.exists = true,
		.is_dir = RIDX_IS_DIR(VFS_NODE_CAST(VFSResIndexNode, node)->index_entry),
		.is_readonly = true,
	};
}

static VFSNode *vfs_resindex_locate(VFSNode *root, const char *path) {
	assert(*path);

	auto rindoe = VFS_NODE_CAST(VFSResIndexNode, root);

	if(!RIDX_IS_DIR(rindoe->index_entry)) {
		vfs_set_error("Not a directory: %s", RIDX_AS_FILE(rindoe->index_entry)->name);
		return NULL;
	}

	const RIdxDirEntry *parent = RIDX_AS_DIR(rindoe->index_entry);

	char path_copy[strlen(path) + 1], *lpath, *rpath;
	memcpy(path_copy, path, sizeof(path_copy));
	vfs_path_split_left(path_copy, &lpath, &rpath);

	for(;;) {
		void *result = (void*)ridx_subdir_lookup(parent, lpath);

		if(!result) {
			result = (void*)ridx_file_lookup(parent, lpath);

			if(*rpath && result) {
				// non-final component is a file
				vfs_set_error("Not a directory: %s", RIDX_AS_FILE(result)->name);
				return NULL;
			}
		}

		if(!result) {
			vfs_set_error("No such file or directory: %s", lpath);
			return NULL;
		}

		if(!*rpath) {
			return &ridx_alloc_node(rindoe, result)->as_generic;
		}

		parent = result;
		vfs_path_split_left(rpath, &lpath, &rpath);
	}
}

static const char *vfs_resindex_iter(VFSNode *node, void **opaque) {
	const RIdxDirEntry *dir;

	auto rindoe = VFS_NODE_CAST(VFSResIndexNode, node);

	if(!RIDX_IS_DIR(rindoe->index_entry)) {
		return NULL;
	}

	dir = RIDX_AS_DIR(rindoe->index_entry);
	intptr_t *iter = (intptr_t *)opaque;
	assert(*iter >= 0);

	int i = *iter;
	*iter = i + 1;

	if(i < dir->subdirs_num) {
		return ridx_dirs[dir->subdirs_ofs + i].name;
	}

	if(i - dir->subdirs_num < dir->files_num) {
		return ridx_files[dir->files_ofs + i - dir->subdirs_num].name;
	}

	return NULL;
}

static char *vfs_resindex_repr(VFSNode *node) {
	auto rinode = VFS_NODE_CAST(VFSResIndexNode, node);

	if(RIDX_IS_DIR(rinode->index_entry)) {
		const RIdxDirEntry *d = RIDX_AS_DIR(rinode->index_entry);
		return strfmt(
			"resource index directory #%i (%s)",
			(int)(d - ridx_dirs), ridx_dir_name(d, "<root>")
		);
	} else {
		const RIdxFileEntry *f = RIDX_AS_FILE(rinode->index_entry);
		return strfmt(
			"resource index file #%i: %s (%s)",
			(int)(f - ridx_files), NOT_NULL(f->content_id), NOT_NULL(f->name)
		);
	}
}

static void vfs_resindex_free(VFSNode *node) {
	auto rinode = VFS_NODE_CAST(VFSResIndexNode, node);

	if(ridx_node_is_root(rinode)) {
		VFSResIndexFSContext *ctx = NOT_NULL(rinode->index_entry);
		NOT_NULL(ctx->procs.free)(ctx);
	} else {
		vfs_decref(rinode->root_node);
	}
}

static SDL_IOStream *vfs_resindex_open(VFSNode *node, VFSOpenMode mode) {
	if(mode & VFS_MODE_WRITE) {
		vfs_set_error("Read-only filesystem");
		return NULL;
	}

	auto rinode = VFS_NODE_CAST(VFSResIndexNode, node);

	if(RIDX_IS_DIR(rinode->root_node)) {
		vfs_set_error("Is a directory: %s", RIDX_AS_DIR(rinode->root_node)->name);
		return NULL;
	}

	const RIdxFileEntry *f = RIDX_AS_FILE(rinode->index_entry);
	VFSResIndexFSContext *ctx = NOT_NULL(rinode->context);
	return NOT_NULL(ctx->procs.open)(ctx, NOT_NULL(f->content_id), mode);
}

VFS_NODE_FUNCS(VFSResIndexNode, {
	.free = vfs_resindex_free,
	.iter = vfs_resindex_iter,
	.locate = vfs_resindex_locate,
	.open = vfs_resindex_open,
	.query = vfs_resindex_query,
	.repr = vfs_resindex_repr,
});

static VFSResIndexNode *ridx_alloc_node(VFSResIndexNode *parent, void *content) {
	auto root = NOT_NULL(parent->root_node);
	assert(ridx_node_is_root(root));
	vfs_incref(root);

	return VFS_ALLOC(VFSResIndexNode, {
		.root_node = root,
		.index_entry = content,
		.context = NOT_NULL(parent->context),
	});
}

VFSNode *vfs_resindex_create(VFSResIndexFSContext *ctx) {
	auto n = VFS_ALLOC(VFSResIndexNode, {
		.context = ctx,
		.index_entry = (void*)&ridx_dirs[0],
	});

	n->root_node = n;
	assert(ridx_node_is_root(n));

	return &n->as_generic;
}
