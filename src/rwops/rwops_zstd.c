/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_zstd.h"
#include "rwops_util.h"

#include "log.h"
#include "util/miscmath.h"

#include <zstd.h>

typedef struct ZstdData {
	SDL_IOStream *wrapped;
	SDL_IOStream *iostream;
	int64_t pos;

	union {
		struct {
			ZSTD_CStream *stream;
			ZSTD_inBuffer in_buffer;
			size_t in_buffer_alloc_size;
			ZSTD_outBuffer out_buffer;
		} writer;

		struct {
			ZSTD_DStream *stream;
			ZSTD_inBuffer in_buffer;
			size_t in_buffer_alloc_size;
			size_t next_read_size;
			int64_t uncompressed_size;
		} reader;
	};

	bool autoclose;
} ZstdData;

static int64_t rwzstd_seek_emulated(void *ctx, int64_t offset, SDL_IOWhence whence);

static int64_t rwzstd_seek(void *ctx, int64_t offset, SDL_IOWhence whence) {
	ZstdData *zdata = ctx;

	if(!offset && whence == SDL_IO_SEEK_CUR) {
		return zdata->pos;
	}

	return SDL_SetError("Can't seek in a zstd stream");
}

static int64_t rwzstd_size(void *ctx) {
	ZstdData *zdata = ctx;
	int64_t size = zdata->reader.uncompressed_size;

	if(size < 0) {
		return SDL_SetError("Uncompressed stream size is unknown");
	}

	return size;
}

static bool rwzstd_reader_close(void *ctx);
static bool rwzstd_writer_close(void *ctx);

static int rwzstd_close(void *ctx) {
	ZstdData *zdata = ctx;

	if(zdata->autoclose) {
		SDL_CloseIO(zdata->wrapped);
	}

	mem_free(zdata);
	return 0;
}

static size_t rwzstd_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status);

static size_t rwzstd_read_invalid(void *ctx, void *ptr, size_t size, SDL_IOStatus *status) {
	SDL_SetError("Can't read from a zstd writer stream");
	*status = SDL_IO_STATUS_WRITEONLY;
	return 0;
}

static size_t rwzstd_write(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status);

static size_t rwzstd_write_invalid(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status) {
	SDL_SetError("Can't write to a zstd reader stream");
	*status = SDL_IO_STATUS_READONLY;
	return 0;
}

static SDL_IOStream *rwzstd_alloc(
	ZstdData **out_zdata, SDL_IOStream *wrapped, bool autoclose, bool writer, bool emulate_seek
) {
	SDL_IOStreamInterface iface = {
		.version = sizeof(iface),
		.seek = emulate_seek ? rwzstd_seek_emulated : rwzstd_seek,
		.size = rwzstd_size,
	};

	if(writer) {
		iface.write = rwzstd_write;
		iface.read = rwzstd_read_invalid;
		iface.close = rwzstd_writer_close;
		iface.seek = rwzstd_seek;
		assume(!emulate_seek);
	} else {
		iface.write = rwzstd_write_invalid;
		iface.read = rwzstd_read;
		iface.close = rwzstd_reader_close;
	}

	auto zdata = ALLOC(ZstdData, {
		.wrapped = wrapped,
		.autoclose = autoclose,
	});


	SDL_IOStream *io = SDL_OpenIO(&iface, zdata);

	if(!io) {
		mem_free(zdata);
	} else {
		zdata->iostream = io;
		*out_zdata = zdata;
	}

	return io;
}

static size_t rwzstd_reader_fill_in_buffer(ZstdData *z, size_t request_size, SDL_IOStatus *status) {
	ZSTD_inBuffer *in = &z->reader.in_buffer;

	if(request_size > z->reader.in_buffer_alloc_size) {
		z->reader.in_buffer_alloc_size = request_size;
		in->src = mem_realloc((void*)in->src, request_size);
	}

	if(request_size == 0) {
		return 0;
	}

	size_t leftover_size = in->size - in->pos;
	in->size = leftover_size;
	assert(leftover_size <= request_size);

	uint8_t *root = (uint8_t*)in->src;

	if(UNLIKELY(leftover_size)) {
		// some input has not been consumed; we must present it again
		uint8_t *leftover = root + in->pos;
		memmove(root, leftover, leftover_size);
	}

	in->pos = 0;

	uint8_t *start = root + in->size;
	uint8_t *end = root + request_size;
	uint8_t *pos = start;

	while(in->size < request_size) {
		size_t read = SDL_ReadIO(z->wrapped, pos, (end - pos));
		*status = SDL_GetIOStatus(z->wrapped);  // FIXME not sure if correct

		if(read == 0) {
			break;
		}

		assert(read <= end - pos);

		pos += read;
		in->size += read;
	}

	assert(in->size <= z->reader.in_buffer_alloc_size);
	return pos - start;
}

static size_t rwzstd_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status) {
	ZstdData *z = ctx;
	ZSTD_inBuffer *in = &z->reader.in_buffer;
	ZSTD_outBuffer out = {
		.dst = ptr,
		.size = size,
	};
	ZSTD_DStream *stream = NOT_NULL(z->reader.stream);

	bool have_input = true;

	while(out.pos < out.size && have_input) {
		if(in->size - in->pos < z->reader.next_read_size) {
			rwzstd_reader_fill_in_buffer(z, z->reader.next_read_size, status);

			if(in->size - in->pos < z->reader.next_read_size) {
				// end of stream
				have_input = false;
			}
		}

		size_t zstatus = ZSTD_decompressStream(stream, &out, in);

		if(UNLIKELY(ZSTD_isError(zstatus))) {
			SDL_SetError("ZSTD_decompressStream() failed: %s", ZSTD_getErrorName(zstatus));
			log_debug("%s", SDL_GetError());
			return 0;
		}

		z->reader.next_read_size = zstatus;
	}

	z->pos += out.pos;
	return out.pos;
}

static bool rwzstd_reader_close(void *ctx) {
	ZstdData *z = ctx;
	ZSTD_freeDStream(z->reader.stream);
	mem_free((void*)z->reader.in_buffer.src);
	return rwzstd_close(z);
}

static SDL_IOStream *rwzstd_open_reader(
	SDL_IOStream *src, bool autoclose, bool seekable, int64_t uncompressed_size
) {
	if(!src) {
		return NULL;
	}

	ZstdData *z;
	SDL_IOStream *io = rwzstd_alloc(&z, src, autoclose, false, false);

	if(!io) {
		return NULL;
	}

	assume(z != NULL);

	z->reader.stream = NOT_NULL(ZSTD_createDStream());
	z->reader.next_read_size = ZSTD_initDStream(z->reader.stream);
	z->reader.in_buffer_alloc_size = max(z->reader.next_read_size, 16384);
	z->reader.in_buffer.src = mem_alloc(z->reader.in_buffer_alloc_size);
	z->reader.next_read_size = z->reader.in_buffer_alloc_size;

	if(seekable && uncompressed_size < 0) {
		// Try to get it from the zstd frame header.
		// This won't work correctly for multi-frame content.
		// The content size may also not be present in the header.

		size_t psize = z->reader.in_buffer.size;
		size_t header_size = 24;  // a bit larger than ZSTD_FRAMEHEADERSIZE_MAX from private API
		SDL_IOStatus status;  // FIXME
		rwzstd_reader_fill_in_buffer(z, header_size, &status);
		z->reader.next_read_size -= z->reader.in_buffer.size - psize;

		ZSTD_inBuffer *in = &z->reader.in_buffer;
		uint8_t *h = (uint8_t*)in->src + in->pos;

		unsigned long long fcs = ZSTD_getFrameContentSize(h, in->size - in->pos);

		if(fcs == ZSTD_CONTENTSIZE_ERROR) {
			log_error("Error getting frame content size from zstd stream");
		} else if(fcs == ZSTD_CONTENTSIZE_UNKNOWN) {
			log_warn("zstd frame content size is unknown");
		} else if(fcs > INT64_MAX) {
			log_warn("zstd frame content size is too large");
		} else {
			uncompressed_size = fcs;
		}
	}

	z->reader.uncompressed_size = uncompressed_size;

	return io;

}

SDL_IOStream *SDL_RWWrapZstdReader(SDL_IOStream *src, bool autoclose) {
	return rwzstd_open_reader(src, autoclose, false, -1);
}

static int rwzstd_reopen(void *ctx) {
	ZstdData *z = ctx;
	int64_t srcpos = SDL_SeekIO(z->wrapped, 0, SDL_IO_SEEK_SET);

	if(srcpos < 0) {
		return srcpos;
	}

	assert(srcpos == 0);

	z->pos = 0;
	z->reader.in_buffer.pos = 0;
	z->reader.in_buffer.size = 0;
	z->reader.next_read_size = ZSTD_initDStream(z->reader.stream);

	return 0;
}

static int64_t rwzstd_seek_emulated(void *ctx, int64_t offset, SDL_IOWhence whence) {
	ZstdData *z = ctx;
	char buf[1024];

	return rwutil_seek_emulated(
		z->iostream, offset, whence,
		&z->pos, rwzstd_reopen, z, sizeof(buf), buf
	);
}

SDL_IOStream *SDL_RWWrapZstdReaderSeekable(
	SDL_IOStream *src, int64_t uncompressed_size, bool autoclose
) {
	return rwzstd_open_reader(src, autoclose, true, uncompressed_size);
}

static bool rwzstd_compress(ZstdData *z, ZSTD_EndDirective edir, size_t *status) {
	ZSTD_outBuffer *out = &z->writer.out_buffer;
	ZSTD_inBuffer *in = &z->reader.in_buffer;

	*status = ZSTD_compressStream2(z->writer.stream, out, in, edir);

	if(UNLIKELY(ZSTD_isError(*status))) {
		SDL_SetError("ZSTD_compressStream2() failed: %s", ZSTD_getErrorName(*status));
		log_debug("%s", SDL_GetError());
		return false;
	}

	if(out->pos > 0) {
		size_t written = SDL_WriteIO(z->wrapped, out->dst, out->pos);

		if(UNLIKELY(written != out->pos)) {
			SDL_SetError("SDL_WriteIO() returned %zi; expected %zi. Error: %s", written, out->pos, SDL_GetError());
			log_debug("%s", SDL_GetError());
			return false;
		}

		out->pos = 0;
	}

	return true;
}

static size_t rwzstd_write(void *ctx, const void *data_in, size_t size, SDL_IOStatus *status) {
	ZstdData *z = ctx;
	ZSTD_inBuffer *in = &z->writer.in_buffer;
	uint8_t *in_root = (uint8_t*)in->src;

	assert(in->pos <= in->size);

	if(in->pos > 0) {
		in->size -= in->pos;
		memmove(in_root, in_root + in->pos, in->size);
		in->pos = 0;
	}

	size_t in_free = z->writer.in_buffer_alloc_size - in->size;

	if(UNLIKELY(in_free < size)) {
		in_root = mem_realloc(in_root, z->writer.in_buffer_alloc_size + (size - in_free));
		in->src = in_root;
	}

	memcpy(in_root + in->size, data_in, size);
	in->size += size;
	z->pos += size;

	size_t zstatus;

	if(LIKELY(rwzstd_compress(z, ZSTD_e_continue, &zstatus))) {
		return size;
	}

	*status = SDL_IO_STATUS_ERROR;
	return 0;
}

static bool rwzstd_writer_flush(void *ctx, ZSTD_EndDirective edir) {
	ZstdData *z = ctx;
	size_t status;

	do {
		if(UNLIKELY(!rwzstd_compress(z, edir, &status))) {
			return false;
		}
	} while(status != 0);

	return true;
}

static bool rwzstd_writer_close(void *ctx) {
	ZstdData *z = ctx;
	bool flush_ok = rwzstd_writer_flush(z, ZSTD_e_end);

	ZSTD_freeCStream(z->writer.stream);
	mem_free(z->writer.out_buffer.dst);
	mem_free((void*)z->writer.in_buffer.src);

	bool r = rwzstd_close(z);
	return r && flush_ok;
}

SDL_IOStream *SDL_RWWrapZstdWriter(SDL_IOStream *src, int clevel, bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	ZstdData *z;
	SDL_IOStream *io = rwzstd_alloc(&z, src, autoclose, true, false);

	if(UNLIKELY(!io)) {
		return NULL;
	}

	z->writer.stream = NOT_NULL(ZSTD_createCStream());
	z->writer.out_buffer.size = ZSTD_CStreamOutSize();
	z->writer.out_buffer.dst = mem_alloc(z->writer.out_buffer.size);
	z->writer.in_buffer_alloc_size = ZSTD_CStreamInSize();
	z->writer.in_buffer.src = mem_alloc(z->writer.in_buffer_alloc_size);

	if(UNLIKELY(clevel < ZSTD_minCLevel() || clevel > ZSTD_maxCLevel())) {
		log_warn("Invalid compression level %i", clevel);
		clevel = RW_ZSTD_LEVEL_DEFAULT;
		assert(0);
	}

	ZSTD_CCtx_setParameter(z->writer.stream, ZSTD_c_compressionLevel, clevel);

	return io;
}
