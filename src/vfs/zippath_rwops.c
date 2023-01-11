/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "zipfile_impl.h"

#include "rwops/rwops_util.h"
#include "rwops/rwops_zstd.h"

#include "util.h"

#define FORCE_MANUAL_DECOMPRESSION (0)

#if FORCE_MANUAL_DECOMPRESSION
	#include "rwops_zlib.h"
#endif

typedef struct ZipRWData {
	zip_file_t *file;
	int64_t pos;
	int64_t size;
	zip_flags_t open_flags;
} ZipRWData;

#define ZTLS(pdata) vfs_zipfile_get_tls((pdata)->zipnode, true)

static int ziprw_reopen(SDL_RWops *rw) {
	ZipRWData *rwdata = rw->hidden.unknown.data1;
	VFSZipPathNode *zpnode = rw->hidden.unknown.data2;

	if(rwdata->file) {
		zip_fclose(rwdata->file);
	}

	rwdata->file = zip_fopen_index(ZTLS(zpnode)->zip, zpnode->index, rwdata->open_flags);
	rwdata->pos = 0;

	return rwdata->file ? 0 : -1;
}

static zip_file_t *ziprw_get_zipfile(SDL_RWops *rw) {
	ZipRWData *rwdata = rw->hidden.unknown.data1;
	VFSZipPathNode *zpnode = rw->hidden.unknown.data2;

	int64_t pos = rwdata->pos;

	if(!vfs_zipfile_get_tls(zpnode->zipnode, false) && rwdata->file) {
		zip_fclose(rwdata->file);
		rwdata->file = NULL;
	}

	if(!rwdata->file && ziprw_reopen(rw) >= 0) {
		SDL_RWseek(rw, pos, RW_SEEK_SET);
	}

	assert(rwdata->file);
	return rwdata->file;
}

static int ziprw_close(SDL_RWops *rw) {
	if(rw) {
		ZipRWData *rwdata = rw->hidden.unknown.data1;
		VFSZipPathNode *zpnode = rw->hidden.unknown.data2;

		if(rwdata->file) {
			zip_fclose(rwdata->file);
		}

		vfs_decref(zpnode);
		mem_free(rwdata);
		SDL_FreeRW(rw);
	}

	return 0;
}

static int64_t ziprw_seek(SDL_RWops *rw, int64_t offset, int whence) {
	ZipRWData *rwdata = rw->hidden.unknown.data1;

	if(!ziprw_get_zipfile(rw)) {
		return -1;
	}

	if(zip_fseek(rwdata->file, offset, whence) == 0) {
		return rwdata->pos = zip_ftell(rwdata->file);
	}

	SDL_SetError("ZIP error: %s", zip_error_strerror(zip_file_get_error(rwdata->file)));
	return -1;
}

static int64_t ziprw_seek_emulated(SDL_RWops *rw, int64_t offset, int whence) {
	ZipRWData *rwdata = rw->hidden.unknown.data1;

	if(!ziprw_get_zipfile(rw)) {
		return -1;
	}

	char buf[1024];

	return rwutil_seek_emulated(
		rw, offset, whence,
		&rwdata->pos, ziprw_reopen, sizeof(buf), buf
	);
}

static int64_t ziprw_size(SDL_RWops *rw) {
	ZipRWData *rwdata = rw->hidden.unknown.data1;

	if(rwdata->size < 0) {
		return SDL_SetError("zip_stat_index() failed");
	}

	return rwdata->size;
}

static size_t ziprw_read(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum) {
	ZipRWData *rwdata = rw->hidden.unknown.data1;

libzip_sucks:

	if(UNLIKELY(!ziprw_get_zipfile(rw))) {
		return 0;
	}

	size_t read_size = size * maxnum;

	if(UNLIKELY(read_size == 0)) {
		return 0;
	}

	if(UNLIKELY(read_size / size != maxnum)) {
		SDL_SetError("Read size is too large");
		return 0;
	}

	zip_int64_t bytes_read = zip_fread(rwdata->file, ptr, read_size);

	if(UNLIKELY(bytes_read < 0)) {
		SDL_SetError("ZIP error: %s", zip_error_strerror(zip_file_get_error(rwdata->file)));
		log_debug("ZIP error: %s", zip_error_strerror(zip_file_get_error(rwdata->file)));
		return 0;
	}

	if(read_size > 0 && bytes_read == 0 && rw->seek == ziprw_seek && zip_ftell(rwdata->file) < rwdata->size) {
		log_debug("libzip BUG: EOF flag not cleared after seek, reopening file");
		zip_fclose(rwdata->file);
		rwdata->file = NULL;
		goto libzip_sucks;
	}

	rwdata->pos += bytes_read;
	return bytes_read / size;
}

static size_t ziprw_write(SDL_RWops *rw, const void *ptr, size_t size, size_t maxnum) {
	SDL_SetError("Not implemented");
	return -1;
}

SDL_RWops *vfs_zippath_make_rwops(VFSZipPathNode *zpnode) {
	SDL_RWops *rw = SDL_AllocRW();

	if(UNLIKELY(!rw)) {
		return NULL;
	}

	memset(rw, 0, sizeof(SDL_RWops));

	auto rwdata = ALLOC(ZipRWData, {
		.size = zpnode->size,
	});

	vfs_incref(zpnode);

	rw->hidden.unknown.data1 = rwdata;
	rw->hidden.unknown.data2 = zpnode;
	rw->type = SDL_RWOPS_UNKNOWN;

	rw->size = ziprw_size;
	rw->close = ziprw_close;
	rw->read = ziprw_read;
	rw->write = ziprw_write;

	DIAGNOSTIC(push)
	DIAGNOSTIC(ignored "-Wunreachable-code")

	if(zpnode->compression == ZIP_CM_STORE) {
		rw->seek = ziprw_seek;
	} else if(
		!FORCE_MANUAL_DECOMPRESSION &&
		zip_compression_method_supported(zpnode->compression, false)
	) {
		rw->seek = ziprw_seek_emulated;
	} else {
		rw->seek = ziprw_seek;
		rwdata->size = zpnode->compressed_size;
		rwdata->open_flags = ZIP_FL_COMPRESSED;

		switch(zpnode->compression) {
			#if FORCE_MANUAL_DECOMPRESSION
			case ZIP_CM_DEFLATE:
				rw = SDL_RWWrapInflateReaderSeekable(rw, zpnode->size, imin(4096, pdata->size), true);
				break;
			#endif

			case ZIP_CM_ZSTD: {
				rw = SDL_RWWrapZstdReaderSeekable(rw, zpnode->size, true);
				break;
			}

			default: {
				char *fname = vfs_node_repr(zpnode, true);
				SDL_SetError("%s: unsupported compression method: %i", fname, zpnode->compression);
				SDL_RWclose(rw);
				return NULL;
			}
		}
	}

	DIAGNOSTIC(pop)

	return rw;
}
