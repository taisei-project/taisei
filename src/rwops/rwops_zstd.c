/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "rwops_zstd.h"
#include "rwops_util.h"
#include "util.h"

#include <zstd.h>

typedef struct ZstdData {
	SDL_RWops *wrapped;
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

#define ZDATA(rw) NOT_NULL((ZstdData*)(NOT_NULL(rw)->hidden.unknown.data1))

static int64_t rwzstd_seek(SDL_RWops *rw, int64_t offset, int whence) {
	if(!offset && whence == RW_SEEK_CUR) {
		return ZDATA(rw)->pos;
	}

	return SDL_SetError("Can't seek in a zstd stream");
}

static int64_t rwzstd_size_invalid(SDL_RWops *rw) {
	return SDL_SetError("Can't get size of a zstd stream");
}

static int64_t rwzstd_size(SDL_RWops *rw) {
	return ZDATA(rw)->reader.uncompressed_size;
}

static int rwzstd_close(SDL_RWops *rw) {
	ZstdData *z = ZDATA(rw);

	if(z->autoclose) {
		SDL_RWclose(z->wrapped);
	}

	mem_free(z);
	SDL_FreeRW(rw);

	return 0;
}

static size_t rwzstd_read_invalid(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum) {
	SDL_SetError("Can't read from a zstd writer stream");
	return 0;
}

static size_t rwzstd_write_invalid(SDL_RWops *rw, const void *ptr, size_t size, size_t maxnum) {
	SDL_SetError("Can't write to a zstd reader stream");
	return 0;
}

static SDL_RWops *rwzstd_alloc(SDL_RWops *wrapped, bool autoclose) {
	SDL_RWops *rw = SDL_AllocRW();

	if(!rw) {
		return NULL;
	}

	memset(rw, 0, sizeof(SDL_RWops));

	rw->type = SDL_RWOPS_UNKNOWN;
	rw->seek = rwzstd_seek;
	rw->size = rwzstd_size_invalid;
	rw->hidden.unknown.data1 = ALLOC(ZstdData, {
		.wrapped = wrapped,
		.autoclose = autoclose,
	});

	return rw;
}

static size_t rwzstd_reader_fill_in_buffer(ZstdData *z, size_t request_size) {
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
		size_t read = SDL_RWread(z->wrapped, pos, 1, end - pos);

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

static size_t rwzstd_read(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum) {
	ZstdData *z = ZDATA(rw);
	ZSTD_inBuffer *in = &z->reader.in_buffer;
	ZSTD_outBuffer out = {
		.dst = ptr,
		.size = size * maxnum,
	};
	ZSTD_DStream *stream = NOT_NULL(z->reader.stream);

	bool have_input = true;

	while(out.pos < out.size && have_input) {
		if(in->size - in->pos < z->reader.next_read_size) {
			rwzstd_reader_fill_in_buffer(z, z->reader.next_read_size);

			if(in->size - in->pos < z->reader.next_read_size) {
				// end of stream
				have_input = false;
			}
		}

		size_t status = ZSTD_decompressStream(stream, &out, in);

		if(UNLIKELY(ZSTD_isError(status))) {
			SDL_SetError("ZSTD_decompressStream() failed: %s", ZSTD_getErrorName(status));
			log_debug("%s", SDL_GetError());
			return 0;
		}

		z->reader.next_read_size = status;
	}

	z->pos += out.pos;
	return out.pos / size;
}

static int rwzstd_reader_close(SDL_RWops *rw) {
	ZstdData *z = ZDATA(rw);
	ZSTD_freeDStream(z->reader.stream);
	mem_free((void*)z->reader.in_buffer.src);
	return rwzstd_close(rw);
}

SDL_RWops *SDL_RWWrapZstdReader(SDL_RWops *src, bool autoclose) {
	if(!src) {
		return NULL;
	}

	SDL_RWops *rw = rwzstd_alloc(src, autoclose);

	if(!rw) {
		return NULL;
	}

	rw->write = rwzstd_write_invalid;
	rw->read = rwzstd_read;
	rw->close = rwzstd_reader_close;

	ZstdData *z = ZDATA(rw);
	z->reader.stream = NOT_NULL(ZSTD_createDStream());
	z->reader.next_read_size = ZSTD_initDStream(z->reader.stream);
	z->reader.in_buffer_alloc_size = imax(z->reader.next_read_size, 16384);
	z->reader.in_buffer.src = mem_alloc(z->reader.in_buffer_alloc_size);
	z->reader.next_read_size = z->reader.in_buffer_alloc_size;

	return rw;
}

static int rwzstd_reopen(SDL_RWops *rw) {
	ZstdData *z = ZDATA(rw);

	int64_t srcpos = SDL_RWseek(z->wrapped, 0, RW_SEEK_SET);

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

static int64_t rwzstd_seek_emulated(SDL_RWops *rw, int64_t offset, int whence) {
	ZstdData *z = ZDATA(rw);
	char buf[1024];

	return rwutil_seek_emulated(
		rw, offset, whence,
		&z->pos, rwzstd_reopen, sizeof(buf), buf
	);
}

SDL_RWops *SDL_RWWrapZstdReaderSeekable(SDL_RWops *src, int64_t uncompressed_size, bool autoclose) {
	SDL_RWops *rw = SDL_RWWrapZstdReader(src, autoclose);

	if(!rw) {
		return NULL;
	}

	rw->seek = rwzstd_seek_emulated;
	ZstdData *z = ZDATA(rw);

	if(uncompressed_size < 0) {
		// Try to get it from the zstd frame header.
		// This won't work correctly for multi-frame content.
		// The content size may also not be present in the header.

		size_t psize = z->reader.in_buffer.size;
		size_t header_size = 24;  // a bit larger than ZSTD_FRAMEHEADERSIZE_MAX from private API
		rwzstd_reader_fill_in_buffer(z, header_size);
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

	if(uncompressed_size >= 0) {
		rw->size = rwzstd_size;
		z->reader.uncompressed_size = uncompressed_size;
	}

	return rw;
}

static bool rwzstd_compress(SDL_RWops *rw, ZSTD_EndDirective edir, size_t *status) {
	ZstdData *z = ZDATA(rw);
	ZSTD_outBuffer *out = &z->writer.out_buffer;
	ZSTD_inBuffer *in = &z->reader.in_buffer;

	*status = ZSTD_compressStream2(z->writer.stream, out, in, edir);

	if(UNLIKELY(ZSTD_isError(*status))) {
		SDL_SetError("ZSTD_compressStream2() failed: %s", ZSTD_getErrorName(*status));
		log_debug("%s", SDL_GetError());
		return false;
	}

	if(out->pos > 0) {
		size_t written = SDL_RWwrite(z->wrapped, out->dst, 1, out->pos);

		if(UNLIKELY(written != out->pos)) {
			SDL_SetError("SDL_RWwrite() returned %zi; expected %zi. Error: %s", written, out->pos, SDL_GetError());
			log_debug("%s", SDL_GetError());
			return false;
		}

		out->pos = 0;
	}

	return true;
}

static size_t rwzstd_write(SDL_RWops *rw, const void *data_in, size_t size, size_t maxnum) {
	ZstdData *z = ZDATA(rw);
	size_t wsize = size * maxnum;

	ZSTD_inBuffer *in = &z->writer.in_buffer;
	uint8_t *in_root = (uint8_t*)in->src;

	assert(in->pos <= in->size);

	if(in->pos > 0) {
		in->size -= in->pos;
		memmove(in_root, in_root + in->pos, in->size);
		in->pos = 0;
	}

	size_t in_free = z->writer.in_buffer_alloc_size - in->size;

	if(UNLIKELY(in_free < wsize)) {
		in_root = mem_realloc(in_root, z->writer.in_buffer_alloc_size + (wsize - in_free));
		in->src = in_root;
	}

	memcpy(in_root + in->size, data_in, wsize);
	in->size += wsize;
	z->pos += wsize;

	size_t status;

	if(LIKELY(rwzstd_compress(rw, ZSTD_e_continue, &status))) {
		return maxnum;
	}

	return 0;
}

static bool rwzstd_writer_flush(SDL_RWops *rw, ZSTD_EndDirective edir) {
	size_t status;

	do {
		if(UNLIKELY(!rwzstd_compress(rw, edir, &status))) {
			return false;
		}
	} while(status != 0);

	return true;
}

static int rwzstd_writer_close(SDL_RWops *rw) {
	ZstdData *z = ZDATA(rw);

	bool flush_ok = rwzstd_writer_flush(rw, ZSTD_e_end);

	ZSTD_freeCStream(z->writer.stream);
	mem_free(z->writer.out_buffer.dst);
	mem_free((void*)z->writer.in_buffer.src);

	int r = rwzstd_close(rw);

	if(!flush_ok && r >= 0) {
		return -1;
	}

	return r;
}

SDL_RWops *SDL_RWWrapZstdWriter(SDL_RWops *src, int clevel, bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	SDL_RWops *rw = rwzstd_alloc(src, autoclose);

	if(UNLIKELY(!rw)) {
		return NULL;
	}

	rw->write = rwzstd_write;
	rw->read = rwzstd_read_invalid;
	rw->close = rwzstd_writer_close;

	ZstdData *z = ZDATA(rw);
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

	return rw;
}
