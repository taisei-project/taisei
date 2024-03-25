/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "zipfile_impl.h"

#include "rwops/rwops_util.h"
#include "rwops/rwops_zstd.h"

#define FORCE_MANUAL_DECOMPRESSION (0)

#if FORCE_MANUAL_DECOMPRESSION
	#include "rwops_zlib.h"
#endif

typedef struct ZipIOData {
	SDL_IOStream *io;
	VFSZipPathNode *zpnode;
	zip_file_t *file;
	int64_t pos;
	int64_t size;
	zip_flags_t open_flags;
	bool seek_emulated;
} ZipIOData;

#define ZTLS(pdata) vfs_zipfile_get_tls((pdata)->zipnode, true)

static int ziprw_reopen(void *vdata) {
	ZipIOData *rwdata = vdata;
	VFSZipPathNode *zpnode = rwdata->zpnode;

	if(rwdata->file) {
		zip_fclose(rwdata->file);
	}

	rwdata->file = zip_fopen_index(ZTLS(zpnode)->zip, zpnode->index, rwdata->open_flags);
	rwdata->pos = 0;

	return rwdata->file ? 0 : -1;
}

static zip_file_t *ziprw_get_zipfile(ZipIOData *rwdata) {
	VFSZipPathNode *zpnode = rwdata->zpnode;

	int64_t pos = rwdata->pos;

	if(!vfs_zipfile_get_tls(zpnode->zipnode, false) && rwdata->file) {
		zip_fclose(rwdata->file);
		rwdata->file = NULL;
	}

	if(!rwdata->file && ziprw_reopen(rwdata) >= 0) {
		SDL_SeekIO(rwdata->io, pos, SDL_IO_SEEK_SET);
	}

	assert(rwdata->file);
	return rwdata->file;
}

static bool ziprw_close(void *vdata) {
	ZipIOData *rwdata = vdata;
	VFSZipPathNode *zpnode = rwdata->zpnode;

	bool result = true;

	if(rwdata->file) {
		result = zip_fclose(rwdata->file);
	}

	vfs_decref(zpnode);
	mem_free(rwdata);

	return result;
}

static int64_t ziprw_seek(void *vdata, int64_t offset, SDL_IOWhence whence) {
	ZipIOData *rwdata = vdata;

	if(!ziprw_get_zipfile(rwdata)) {
		return -1;
	}

	if(rwdata->seek_emulated) {
		char buf[1024];

		return rwutil_seek_emulated(
			rwdata->io, offset, whence,
			&rwdata->pos, ziprw_reopen, rwdata,
			sizeof(buf), buf
		);
	}

	if(zip_fseek(rwdata->file, offset, whence) == 0) {
		return rwdata->pos = zip_ftell(rwdata->file);
	}

	SDL_SetError("ZIP error: %s", zip_error_strerror(zip_file_get_error(rwdata->file)));
	return -1;
}

static int64_t ziprw_size(void *vdata) {
	ZipIOData *rwdata = vdata;

	if(rwdata->size < 0) {
		return SDL_SetError("zip_stat_index() failed");
	}

	return rwdata->size;
}

static size_t ziprw_read(void *vdata, void *ptr, size_t read_size, SDL_IOStatus *status) {
	ZipIOData *rwdata = vdata;

libzip_sucks:

	if(UNLIKELY(!ziprw_get_zipfile(rwdata))) {
		*status = SDL_IO_STATUS_ERROR;
		return 0;
	}

	if(UNLIKELY(read_size == 0)) {
		// FIXME: what status here?
		return 0;
	}

	zip_int64_t bytes_read = zip_fread(rwdata->file, ptr, read_size);

	if(UNLIKELY(bytes_read < 0)) {
		SDL_SetError("ZIP error: %s", zip_error_strerror(zip_file_get_error(rwdata->file)));
		log_debug("ZIP error: %s", zip_error_strerror(zip_file_get_error(rwdata->file)));
		*status = SDL_IO_STATUS_ERROR;
		return 0;
	}

	if(
		read_size > 0 &&
		bytes_read == 0 &&
		!rwdata->seek_emulated &&
		zip_ftell(rwdata->file) < rwdata->size
	) {
		log_debug("libzip BUG: EOF flag not cleared after seek, reopening file");
		zip_fclose(rwdata->file);
		rwdata->file = NULL;
		goto libzip_sucks;
	}

	if(read_size > bytes_read) {
		// FIXME: do this properly
		*status = SDL_IO_STATUS_EOF;
	}

	rwdata->pos += bytes_read;
	return bytes_read;
}

static size_t ziprw_write(void *vdata, const void *ptr, size_t size, SDL_IOStatus *status) {
	*status = SDL_IO_STATUS_READONLY;
	return 0;
}

SDL_IOStream *vfs_zippath_make_rwops(VFSZipPathNode *zpnode) {
	auto iodata = ALLOC(ZipIOData, {
		.zpnode = zpnode,
		.size = zpnode->size,
	});

	SDL_IOStreamInterface iface = {
		.version = sizeof(iface),
		.size = ziprw_size,
		.close = ziprw_close,
		.read = ziprw_read,
		.write = ziprw_write,
		.seek = ziprw_seek,
	};

	SDL_IOStream *io = SDL_OpenIO(&iface, iodata);

	if(UNLIKELY(!io)) {
		mem_free(iodata);
		return NULL;
	}

	iodata->io = io;

	vfs_incref(zpnode);

	DIAGNOSTIC(push)
	DIAGNOSTIC(ignored "-Wunreachable-code")

	if(zpnode->compression == ZIP_CM_STORE) {
		iodata->seek_emulated = false;
	} else if(
		!FORCE_MANUAL_DECOMPRESSION &&
		zip_compression_method_supported(zpnode->compression, false)
	) {
		iodata->seek_emulated = true;
	} else {
		iodata->seek_emulated = false;
		iodata->size = zpnode->compressed_size;
		iodata->open_flags = ZIP_FL_COMPRESSED;

		switch(zpnode->compression) {
			#if FORCE_MANUAL_DECOMPRESSION
			case ZIP_CM_DEFLATE:
				io = SDL_RWWrapInflateReaderSeekable(io, zpnode->size, imin(4096, iodata->size), true);
				break;
			#endif

			case ZIP_CM_ZSTD: {
				io = SDL_RWWrapZstdReaderSeekable(io, zpnode->size, true);
				break;
			}

			default: {
				char *fname = vfs_node_repr(zpnode, true);
				SDL_SetError("%s: unsupported compression method: %i", fname, zpnode->compression);
				SDL_CloseIO(io);
				return NULL;
			}
		}
	}

	DIAGNOSTIC(pop)

	return io;
}
