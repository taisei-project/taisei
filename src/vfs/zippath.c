/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "log.h"
#include "memory/scratch.h"
#include "rwops/rwops_util.h"
#include "rwops/rwops_zlib.h"
#include "rwops/rwops_zstd.h"
#include "util/miscmath.h"
#include "zipfile_impl.h"
#include "syspath.h"

static const char *vfs_zippath_name(VFSZipPathNode *zpnode) {
	return zpnode->entry->name;
}

static void vfs_zippath_free(VFSNode *node) {
	auto zpnode = VFS_NODE_CAST(VFSZipPathNode, node);
	vfs_decref(zpnode->znode);
}

static void vfs_zippath_repr(VFSNode *node, StringBuffer *buf) {
	auto zpnode = VFS_NODE_CAST(VFSZipPathNode, node);
	strbuf_printf(buf, "%s '%s' in ",
		zpnode->entry->is_dir ? "directory" : "file", vfs_zippath_name(zpnode));
	vfs_node_repr(zpnode->znode, false, buf);
}

static bool vfs_zippath_syspath(VFSNode *node, StringBuffer *buf) {
	auto zpnode = VFS_NODE_CAST(VFSZipPathNode, node);
	StringBuffer buf2 = { acquire_scratch_arena() };
	vfs_node_repr(zpnode->znode, true, &buf2);
	strbuf_printf(&buf2, "%c%s", vfs_syspath_separators[0], vfs_zippath_name(zpnode));
	vfs_syspath_normalize_buffer(buf2.start, buf);
	release_scratch_arena(buf2.arena);
	return true;
}

static VFSInfo vfs_zippath_query(VFSNode *node) {
	return (VFSInfo) {
		.exists = true,
		.is_readonly = true,
		.is_dir = VFS_NODE_CAST(VFSZipPathNode, node)->entry->is_dir
	};
}

static VFSNode *vfs_zippath_locate(VFSNode *node, const char *path) {
	auto zpnode = VFS_NODE_CAST(VFSZipPathNode, node);
	auto e = zpnode->entry;
	StringBuffer buf = { acquire_scratch_arena() };
	strbuf_printf(&buf, "%.*s/%s", e->name_len, e->name, path);
	vfs_path_normalize_inplace(buf.start);
	auto n = vfs_locate(zpnode->znode, buf.start);
	release_scratch_arena(buf.arena);
	return n;
}

static const char *vfs_zippath_iter(VFSNode *node, void **opaque) {
	auto zpnode = VFS_NODE_CAST(VFSZipPathNode, node);
	VFSZipFileIterData *idata = *opaque;
	auto znode = zpnode->znode;

	if(!zpnode->entry->is_dir) {
		return NULL;
	}

	if(!idata) {
		auto e = zpnode->entry;
		uint32_t buf_size = znode->max_name_len - e->name_len;
		*opaque = idata = ALLOC_FLEX(VFSZipFileIterData, buf_size);
		idata->entry = zpnode->entry + 1;
		idata->prefix = e->name;
		idata->prefix_len = e->name_len;
	}

	return vfs_zipfile_iter_shared(znode, idata);
}

#define vfs_zippath_iter_stop vfs_zipfile_iter_stop

static SDL_IOStream *vfs_zippath_open(VFSNode *node, VFSOpenMode mode) {
	auto zpnode = VFS_NODE_CAST(VFSZipPathNode, node);
	auto znode = zpnode->znode;
	auto entry = zpnode->entry;

	if(mode & VFS_MODE_WRITE) {
		vfs_set_error("ZIP archives are read-only");
		return NULL;
	}

	if(mode & VFS_MODE_SEEKABLE && entry->compression != ZIP_COMPRESSION_NONE) {
		StringBuffer buf = { acquire_scratch_arena() };
		vfs_node_repr(node, true, &buf);
		log_warn("Opening compressed file '%s' in seekable mode, this is suboptimal. Consider storing this file without compression", buf.start);
		release_scratch_arena(buf.arena);
	}

	uint32_t ofs = zipfile_get_entry_data_offset(&znode->ctx, entry);

	if(ofs == ZIP_INVALID_OFFSET) {
		vfs_set_error("%s: Can't read %.*s",
			znode->ctx.log_prefix, entry->name_len, entry->name);
		return NULL;
	}

	// NOTE ideally should incref znode and decref when closed,
	// but in practice the znode will live until VFS is shutdown anwyay,
	// and we should't have any open streams by that point.
	auto io = NOT_NULL(SDL_IOFromConstMem(znode->ctx.mem + ofs, entry->comp_size));

	WITH_SCRATCH(scratch, ({
		StringBuffer buf = { scratch };
		vfs_zippath_syspath(node, &buf);
		auto props = SDL_GetIOProperties(io);
		SDL_SetStringProperty(props, PROP_IOSTREAM_NAME, buf.start);
	}));

	switch(entry->compression) {
		case ZIP_COMPRESSION_NONE:
			break;

		case ZIP_COMPRESSION_DEFLATE:
			io = SDL_RWWrapInflateReaderSeekable(io, entry->uncomp_size, min(4096, entry->comp_size), true);
			break;

		case ZIP_COMPRESSION_ZSTD:
		case ZIP_COMPRESSION_ZSTD_DEPRECATED:
			io = SDL_RWWrapZstdReaderSeekable(io, entry->uncomp_size, true);
			break;

		default:
			vfs_set_error("%s: Can't read %.*s: Unhandled compression mode %d",
				znode->ctx.log_prefix, entry->name_len, entry->name, entry->compression);
			SDL_CloseIO(io);
			return NULL;
	}

	return NOT_NULL(io);
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

VFSNode *vfs_zippath_create(VFSZipNode *zipnode, const ZipEntry *entry) {
	auto zpnode = VFS_ALLOC(VFSZipPathNode, {
		.znode = zipnode,
		.entry = entry,
	});

	vfs_incref(zipnode);
	return &zpnode->as_generic;
}
