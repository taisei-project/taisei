/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "rwops_zipfile.h"
#include "util.h"

typedef struct ZipRWData {
	VFSNode *node;
	zip_file_t *file;
	int64_t pos;
} ZipRWData;

#define ZTLS(pdata) vfs_zipfile_get_tls((pdata)->zipnode, true)

static zip_file_t *ziprw_open(VFSZipPathData *pdata) {
	zip_file_t *zipfile = zip_fopen_index(ZTLS(pdata)->zip, pdata->index, 0);

	if(!zipfile) {
		SDL_SetError("ZIP error: %s", zip_error_strerror(&ZTLS(pdata)->error));
	}

	return zipfile;
}

static zip_file_t *ziprw_get_zipfile(SDL_RWops *rw) {
	ZipRWData *rwdata = rw->hidden.unknown.data1;
	VFSZipPathData *pdata = rw->hidden.unknown.data2;

	int64_t pos = rwdata->pos;

	if(!vfs_zipfile_get_tls((pdata)->zipnode, false) && rwdata->file) {
		zip_fclose(rwdata->file);
		rwdata->file = NULL;
		rwdata->pos = 0;
	}

	if(!rwdata->file) {
		rwdata->file = ziprw_open(pdata);
		SDL_RWseek(rw, pos, RW_SEEK_SET);
	}

	assert(rwdata->file);
	return rwdata->file;
}

static int ziprw_close(SDL_RWops *rw) {
	if(rw) {
		ZipRWData *rwdata = rw->hidden.unknown.data1;

		if(rwdata->file) {
			zip_fclose(rwdata->file);
		}

		vfs_decref(rwdata->node);
		free(rwdata);
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
	VFSZipPathData *pdata = rw->hidden.unknown.data2;

	if(!ziprw_get_zipfile(rw)) {
		return -1;
	}

	ssize_t new_pos;
	ssize_t sz = SDL_RWsize(rw);

	switch(whence) {
		case RW_SEEK_SET: new_pos = offset; break;
		case RW_SEEK_CUR: new_pos = rwdata->pos + offset; break;
		case RW_SEEK_END:
			if(sz < 0) {
				return sz;
			}

			new_pos = sz - offset;
			break;

		default: UNREACHABLE;
	}

	if(new_pos < 0 || new_pos > sz) {
		SDL_SetError("Seek offset out of range");
		return -1;
	}

	if(new_pos > rwdata->pos) {
		const size_t chunk_size = 4096;
		do {
			size_t read_size = imin(new_pos - rwdata->pos, chunk_size);
			uint8_t chunk[read_size];
			size_t actual_read_size = SDL_RWread(rw, chunk, 1, read_size);

			assert(actual_read_size <= read_size);

			if(actual_read_size < read_size) {
				break;
			}
		} while(new_pos > rwdata->pos);
	} else if(new_pos < rwdata->pos) {
		zip_fclose(rwdata->file);
		rwdata->file = ziprw_open(pdata);
		rwdata->pos = 0;
		int64_t s = ziprw_seek_emulated(rw, new_pos, RW_SEEK_SET);
		assert(s == new_pos);
		assert(s == rwdata->pos);
		return s;
	}

	return rwdata->pos;
}

static int64_t ziprw_size(SDL_RWops *rw) {
	VFSZipPathData *pdata = rw->hidden.unknown.data2;

	if(pdata->size < 0) {
		SDL_SetError("zip_stat_index() failed");
		return -1;
	}

	return pdata->size;
}

static size_t ziprw_read(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum) {
	ZipRWData *rwdata = rw->hidden.unknown.data1;
	VFSZipPathData *pdata = rw->hidden.unknown.data2;

libzip_sucks:

	if(!ziprw_get_zipfile(rw)) {
		return 0;
	}

	size_t read_size = size * maxnum;

	if(size != 0 && read_size / size != maxnum) {
		SDL_SetError("Read size is too large");
		return 0;
	}

	zip_int64_t bytes_read = zip_fread(rwdata->file, ptr, read_size);

	if(bytes_read < 0) {
		SDL_SetError("ZIP error: %s", zip_error_strerror(zip_file_get_error(rwdata->file)));
		log_debug("ZIP error: %s", zip_error_strerror(zip_file_get_error(rwdata->file)));
		return 0;
	}

	if(read_size > 0 && bytes_read == 0 && rw->seek == ziprw_seek && zip_ftell(rwdata->file) < pdata->size) {
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

SDL_RWops *SDL_RWFromZipFile(VFSNode *znode, VFSZipPathData *pdata) {
	SDL_RWops *rw = SDL_AllocRW();
	memset(rw, 0, sizeof(SDL_RWops));

	ZipRWData *rwdata = calloc(1, sizeof(*rwdata));
	rwdata->node = znode;

	vfs_incref(znode);

	rw->hidden.unknown.data1 = rwdata;
	rw->hidden.unknown.data2 = pdata;
	rw->type = SDL_RWOPS_UNKNOWN;

	rw->size = ziprw_size;
	rw->close = ziprw_close;
	rw->read = ziprw_read;
	rw->write = ziprw_write;

	if(pdata->compression == ZIP_CM_STORE) {
		rw->seek = ziprw_seek;
	} else {
		rw->seek = ziprw_seek_emulated;
	}

	return rw;
}
