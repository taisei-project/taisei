/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "zipfile.h"
#include "zipfile_impl.h"

#include "log.h"
#include "memory/scratch.h"
#include "util/miscmath.h"

static VFSInfo vfs_zipfile_query(VFSNode *node) {
	return (VFSInfo) {
		.exists = true,
		.is_dir = true,
		.is_readonly = true,
	};
}

static bool vfs_zipfile_syspath(VFSNode *node, StringBuffer *buf) {
	return vfs_node_syspath(VFS_NODE_CAST(VFSZipNode, node)->source, buf);
}

static void vfs_zipfile_repr(VFSNode *node, StringBuffer *buf) {
	auto znode = VFS_NODE_CAST(VFSZipNode, node);
	strbuf_cat(buf, "zip archive ");
	vfs_node_repr(znode->source, false, buf);
}

static int zip_entry_name_cmp(const void *a, const void *b) {
	const ZipEntry *e0 = a;
	const ZipEntry *e1 = b;
	auto common_len = min(e0->name_len, e1->name_len);
	int r = strncmp(e0->name, e1->name, common_len);
	return r ?: (int)e0->name_len - (int)e1->name_len;
}

typedef struct ZipSearchKey {
	const char *name;
	size_t len;
} ZipSearchKey;

static int zip_entry_name_search_cmp(const void *a, const void *b) {
	const ZipSearchKey *key = a;
	const ZipEntry *e = b;
	auto common_len = min(key->len, e->name_len);
	int r = strncmp(key->name, e->name, common_len);
	return r ?: (int)key->len - (int)e->name_len;
}

static const ZipEntry *vfs_zipfile_find_entry(VFSZipNode *znode, const char *name, size_t name_len) {
	return bsearch(&(ZipSearchKey) {
			.name = name,
			.len = name_len,
		}, znode->entries, znode->num_entries, sizeof(*znode->entries), zip_entry_name_search_cmp);
}

static VFSNode *vfs_zipfile_locate(VFSNode *node, const char *path) {
	auto znode = VFS_NODE_CAST(VFSZipNode, node);
	auto entry = vfs_zipfile_find_entry(znode, path, strlen(path));

	if(!entry) {
		return NULL;
	}

	return vfs_zippath_create(znode, entry);
}

const char *vfs_zipfile_iter_shared(const VFSZipNode *znode, VFSZipFileIterData *idata) {
	while(idata->entry < NOT_NULL(znode->entries) + znode->num_entries) {
		auto e = idata->entry;

		const char *name;
		uint32_t name_len;

		if(idata->prefix_len) {
			// Since the entries are sorted, we can stop scanning as soon as
			// we find an entry that does not start with prefix/

			if(e->name_len <= idata->prefix_len) {
				// Too short to be a sub-path of prefix
				return NULL;
			}

			if(e->name[idata->prefix_len] != '/') {
				// Doesn't matter if prefix part matches or not; not a sub-path
				return NULL;
			}

			if(strncmp(e->name, idata->prefix, idata->prefix_len)) {
				// Wrong prefix
				return NULL;
			}

			uint32_t skip_len = 1 + idata->prefix_len;
			name = e->name + skip_len;
			name_len = e->name_len - skip_len;
		} else {
			name = e->name;
			name_len = e->name_len;
		}

		if(
			idata->last_name_len <= name_len &&
			(name[idata->last_name_len] == '/' || name[idata->last_name_len] == '\0') &&
			!strncmp(idata->name_buf, name, idata->last_name_len)
		) {
			// This is a sub-path of a directory we've just reported, skip it
			idata->entry++;
			continue;
		}

		const char *sep = strchr(name, '/');
		if(sep) {
			name_len = sep - name;
		}

		memcpy(idata->name_buf, name, name_len);
		idata->last_name_len = name_len;
		idata->name_buf[name_len] = 0;
		idata->entry++;
		return idata->name_buf;
	}

	return NULL;
}

static const char *vfs_zipfile_iter(VFSNode *node, void **opaque) {
	auto znode = VFS_NODE_CAST(VFSZipNode, node);
	VFSZipFileIterData *idata = *opaque;

	if(!idata) {
		if(UNLIKELY(!znode->entries)) {
			return NULL;
		}

		uint32_t buf_size = znode->max_name_len;
		*opaque = idata = ALLOC_FLEX(VFSZipFileIterData, buf_size);
		idata->entry = znode->entries;
		idata->prefix = "";
		idata->prefix_len = 0;
	}

	return vfs_zipfile_iter_shared(znode, idata);
}

void vfs_zipfile_iter_stop(VFSNode *node, void **opaque) {
	VFSZipFileIterData *idata = *opaque;

	if(idata) {
		mem_free(idata);
		*opaque = NULL;
	}
}

static void vfs_zipfile_free(VFSNode *node) {
	auto znode = VFS_NODE_CAST(VFSZipNode, node);

	if(znode->source) {
		if(vfs_mmap_ticket_valid(znode->mmap_ticket)) {
			vfs_node_munmap(znode->source, znode->mmap_ticket);
		}

		vfs_decref(znode->source);
	}

	marena_deinit(&znode->arena);
}

VFS_NODE_FUNCS(VFSZipNode, {
	.repr = vfs_zipfile_repr,
	.query = vfs_zipfile_query,
	.free = vfs_zipfile_free,
	.syspath = vfs_zipfile_syspath,
	.locate = vfs_zipfile_locate,
	.iter = vfs_zipfile_iter,
	.iter_stop = vfs_zipfile_iter_stop,
});

typedef struct ZipParseContext {
	VFSZipNode *znode;
	uint i;
	StringBuffer name_buf;
} ZipParseContext;

static bool vfs_zipfile_parse_callback(void *userdata, const ZipEntry *e, uint32_t entry_idx, uint32_t num_entries) {
	ZipParseContext *pctx = userdata;
	VFSZipNode *znode = pctx->znode;

	if(!znode->entries) {
		assert(num_entries > 0);
		uint avg_name_len = 32;
		marena_init(&znode->arena, num_entries * (sizeof(*znode->entries) + avg_name_len));
		znode->entries = marena_alloc_array(&znode->arena, num_entries, sizeof(*znode->entries));
		znode->num_entries = num_entries;
		pctx->name_buf = (StringBuffer) { &znode->arena };
	}

	assert(num_entries == znode->num_entries);

	if(UNLIKELY(e->name_len > ZIP_NAME_MAXLEN)) {
		log_warn("%s: Discarded record %d: Name is too long (%d > %d)",
			znode->ctx.log_prefix, entry_idx, e->name_len, ZIP_NAME_MAXLEN);
		return true;
	}

	if(e->name_len > znode->max_name_len) {
		znode->max_name_len = e->name_len;
	}

	char *normalized = vfs_path_normalize_inplace(marena_strndup(&znode->arena, e->name, e->name_len));

	auto new_entry = &znode->entries[pctx->i++];
	*new_entry = *e;
	new_entry->name = normalized;
	new_entry->name_len = strlen(normalized);

	return true;
}

VFSNode *vfs_zipfile_create(VFSNode *source) {
	const void *mem;
	size_t mem_size;
	auto mmap_ticket = vfs_node_mmap(source, &mem, &mem_size, true);

	if(!vfs_mmap_ticket_valid(mmap_ticket)) {
		return NULL;
	}

	auto znode = VFS_ALLOC(VFSZipNode, {
		.mmap_ticket = mmap_ticket,
		.source = source,
		.ctx = {
			.mem = mem,
			.mem_size = mem_size,
		}
	});

	StringBuffer buf = { acquire_scratch_arena() };
	vfs_node_repr(source, true, &buf);
	znode->ctx.log_prefix = strbuf_commit(&buf);

	ZipParseContext ctx = {
		.znode = znode,
	};

	if(!zipfile_parse(&znode->ctx, vfs_zipfile_parse_callback, &ctx)) {
		strbuf_printf(&buf, "%s: Failed to parse ZIP file", znode->ctx.log_prefix);
		vfs_decref(znode);
		vfs_set_error("%s", buf.start);
		release_scratch_arena(buf.arena);
		return NULL;
	}

	znode->ctx.log_prefix = marena_strdup(&znode->arena, znode->ctx.log_prefix);
	release_scratch_arena(buf.arena);

	qsort(znode->entries, znode->num_entries, sizeof(*znode->entries), zip_entry_name_cmp);

	return &znode->as_generic;
}
