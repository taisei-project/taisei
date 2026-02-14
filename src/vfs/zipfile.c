/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "zipfile.h"
#include "zipfile_impl.h"

#define LOG_SDL_ERROR log_debug("SDL error: %s", SDL_GetError())

static zip_int64_t vfs_zipfile_srcfunc(void *userdata, void *data, zip_uint64_t len, zip_source_cmd_t cmd) {
	VFSZipNode *zipnode = userdata;
	VFSZipFileTLS *tls = vfs_zipfile_get_tls(zipnode, false);
	VFSNode *source = zipnode->source;
	zip_int64_t ret = -1;

	if(!tls) {
		return -1;
	}

	switch(cmd) {
		case ZIP_SOURCE_OPEN: {
			tls->stream = vfs_node_open(source, VFS_MODE_READ);

			if(!tls->stream) {
				zip_error_set(&tls->error, ZIP_ER_OPEN, 0);
				return -1;
			}

			return 0;
		}

		case ZIP_SOURCE_CLOSE: {
			if(tls->stream) {
				SDL_CloseIO(tls->stream);
				tls->stream = NULL;
			}
			return 0;
		}

		case ZIP_SOURCE_STAT: {
			zip_stat_t *stat = data;
			zip_stat_init(stat);

			stat->valid = ZIP_STAT_COMP_METHOD | ZIP_STAT_ENCRYPTION_METHOD;
			stat->comp_method = ZIP_CM_STORE;
			stat->encryption_method = ZIP_EM_NONE;

			if(tls->stream) {
				stat->valid |= ZIP_STAT_SIZE | ZIP_STAT_COMP_SIZE;
				stat->size = SDL_GetIOSize(tls->stream);
				stat->comp_size = stat->size;
			}

			return sizeof(struct zip_stat);
		}

		case ZIP_SOURCE_READ: {
			assume(tls->stream != NULL);
			ret = SDL_ReadIO(tls->stream, data, len);
			ret = (ret <= 0) ? 0 : ret;

			if(!ret) {
				LOG_SDL_ERROR;
				zip_error_set(&tls->error, ZIP_ER_READ, 0);
				ret = -1;
			}

			return ret;
		}

		case ZIP_SOURCE_SEEK: {
			struct zip_source_args_seek *s;
			s = ZIP_SOURCE_GET_ARGS(struct zip_source_args_seek, data, len, &tls->error);

			if(UNLIKELY(!s)) {
				return -1;
			}

			assume(tls->stream != NULL);
			ret = SDL_SeekIO(tls->stream, s->offset, s->whence);

			if(ret < 0) {
				LOG_SDL_ERROR;
				zip_error_set(&tls->error, ZIP_ER_SEEK, 0);
			}

			return ret;
		}

		case ZIP_SOURCE_TELL: {
			assume(tls->stream != NULL);
			ret = SDL_TellIO(tls->stream);

			if(ret < 0) {
				LOG_SDL_ERROR;
				zip_error_set(&tls->error, ZIP_ER_TELL, 0);
			}

			return ret;
		}

		case ZIP_SOURCE_ERROR: {
			return zip_error_to_data(&tls->error, data, len);
		}

		case ZIP_SOURCE_SUPPORTS: {
			return ZIP_SOURCE_SUPPORTS_SEEKABLE;
		}

		case ZIP_SOURCE_FREE: {
			return 0;
		}

		default: {
			zip_error_set(&tls->error, ZIP_ER_INTERNAL, 0);
			return -1;
		}
	}
}

static void vfs_zipfile_free_tls(void *vtls) {
	VFSZipFileTLS *tls = vtls;

	if(tls->zip) {
		zip_discard(tls->zip);
	}

	if(tls->stream) {
		SDL_CloseIO(tls->stream);
	}

	mem_free(tls);
}

static void vfs_zipfile_free(VFSNode *node) {
	if(node) {
		auto znode = VFS_NODE_CAST(VFSZipNode, node);
		VFSZipFileTLS *tls = SDL_GetTLS(&znode->tls_id);

		if(tls) {
			vfs_zipfile_free_tls(tls);
			SDL_SetTLS(&znode->tls_id, NULL, NULL);
		}

		if(znode->source) {
			vfs_decref(znode->source);
		}

		ht_destroy(&znode->pathmap);
	}
}

static VFSInfo vfs_zipfile_query(VFSNode *node) {
	return (VFSInfo) {
		.exists = true,
		.is_dir = true,
		.is_readonly = true,
	};
}

static char *vfs_zipfile_syspath(VFSNode *node) {
	return vfs_node_syspath(VFS_NODE_CAST(VFSZipNode, node)->source);
}

static char *vfs_zipfile_repr(VFSNode *node) {
	auto znode = VFS_NODE_CAST(VFSZipNode, node);
	char *srcrepr = vfs_node_repr(znode->source, false);
	char *ziprepr = strjoin("zip archive ", srcrepr, NULL);
	mem_free(srcrepr);
	return ziprepr;
}

static VFSNode *vfs_zipfile_locate(VFSNode *node, const char *path) {
	auto znode = VFS_NODE_CAST(VFSZipNode, node);
	VFSZipFileTLS *tls = vfs_zipfile_get_tls(znode, true);
	int64_t idx;

	if(!ht_lookup(&znode->pathmap, path, &idx)) {
		idx = zip_name_locate(tls->zip, path, 0);
	}

	if(idx < 0) {
		return NULL;
	}

	return vfs_zippath_create(znode, idx);
}

const char *vfs_zipfile_iter_shared(VFSZipFileIterData *idata, VFSZipFileTLS *tls) {
	const char *r = NULL;

	for(; !r && idata->idx < idata->num; ++idata->idx) {
		const char *p = zip_get_name(tls->zip, idata->idx, 0);
		const char *p_original = p;

		if(idata->prefix) {
			if(strstartswith(p, idata->prefix)) {
				p += idata->prefix_len;
			} else {
				continue;
			}
		}

		while(!strncmp(p, "./", 2)) {
			// skip the redundant "current directory" prefix
			p += 2;
		}

		if(!strncmp(p, "../", 3)) {
			// sorry, nope. can't work with this
			log_warn("Bad path in zip file: %s", p_original);
			continue;
		}

		if(!*p) {
			continue;
		}

		const char *sep = strchr(p, '/');

		if(sep) {
			if(*(sep + 1)) {
				// path is inside a subdirectory, we want only top-level entries
				continue;
			}

			// separator is the last character in the string
			// this is a top-level subdirectory
			// strip the trailing slash

			mem_free(idata->allocated);
			idata->allocated = mem_strdup(p);
			*strchr(idata->allocated, '/') = 0;
			r = idata->allocated;
		} else {
			r = p;
		}
	}

	return r;
}

static const char *vfs_zipfile_iter(VFSNode *node, void **opaque) {
	auto znode = VFS_NODE_CAST(VFSZipNode, node);
	VFSZipFileIterData *idata = *opaque;
	VFSZipFileTLS *tls = vfs_zipfile_get_tls(znode, true);

	if(!idata) {
		*opaque = idata = ALLOC(VFSZipFileIterData);
		idata->num = zip_get_num_entries(tls->zip, 0);
	}

	return vfs_zipfile_iter_shared(idata, tls);
}

void vfs_zipfile_iter_stop(VFSNode *node, void **opaque) {
	VFSZipFileIterData *idata = *opaque;

	if(idata) {
		mem_free(idata->allocated);
		mem_free(idata);
		*opaque = NULL;
	}
}

VFS_NODE_FUNCS(VFSZipNode, {
	.repr = vfs_zipfile_repr,
	.query = vfs_zipfile_query,
	.free = vfs_zipfile_free,
	.syspath = vfs_zipfile_syspath,
	//.mount = vfs_zipfile_mount,
	.locate = vfs_zipfile_locate,
	.iter = vfs_zipfile_iter,
	.iter_stop = vfs_zipfile_iter_stop,
	//.mkdir = vfs_zipfile_mkdir,
	//.open = vfs_zipfile_open,
});

static bool vfs_zipfile_init_pathmap(VFSZipNode *znode) {
	VFSZipFileTLS *tls = vfs_zipfile_get_tls(znode, true);

	if(!tls) {
		return false;
	}

	zip_int64_t num = zip_get_num_entries(tls->zip, 0);

	ht_create(&znode->pathmap);

	for(zip_int64_t i = 0; i < num; ++i) {
		const char *original = zip_get_name(tls->zip, i, 0);
		char normalized[strlen(original) + 1];
		vfs_path_normalize(original, normalized);

		if(strcmp(original, normalized)) {
			ht_set(&znode->pathmap, normalized, i);
		}
	}

	return true;
}

VFSZipFileTLS *vfs_zipfile_get_tls(VFSZipNode *znode, bool create) {
	VFSZipFileTLS *tls = SDL_GetTLS(&znode->tls_id);

	if(tls || !create) {
		return tls;
	}

	tls = ALLOC(typeof(*tls));
	SDL_SetTLS(&znode->tls_id, tls, (void(*)(void*))vfs_zipfile_free_tls);

	zip_source_t *src = zip_source_function_create(vfs_zipfile_srcfunc, znode, &tls->error);
	zip_t *zip = tls->zip = zip_open_from_source(src, ZIP_RDONLY, &tls->error);

	// FIXME: Taisei currently doesn't handle zip files without explicit directory entries correctly (file listing will not work)

	if(!zip) {
		char *r = vfs_node_repr(znode->source, true);
		vfs_set_error("Failed to open zip archive '%s': %s", r, zip_error_strerror(&tls->error));
		mem_free(r);
		vfs_zipfile_free_tls(tls);
		SDL_SetTLS(&znode->tls_id, 0, NULL);
		zip_source_free(src);
		return NULL;
	}

	return tls;
}

VFSNode *vfs_zipfile_create(VFSNode *source) {
	auto znode = VFS_ALLOC(VFSZipNode, {
		.source = source,
	});

	if(!vfs_zipfile_init_pathmap(znode)) {
		vfs_decref(znode);
	}

	return &znode->as_generic;
}
