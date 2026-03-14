/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_zstd.h"
#include "rwops_util.h"

#include "log.h"
#include "memory/scratch.h"
#include "util.h"
#include "util/miscmath.h"

#include <zstd.h>

/*
 * This implementation supports reading, but not writing, the Zstandard seekable format.
 *
 * https://github.com/facebook/zstd/tree/v1.5.7/contrib/seekable_format
 *
 * If a seekable stream is requested, we'll try to locate and parse the seek table at the
 * end of the compressed content. If successful, it'll be used to implemet seeking efficiently.
 * A cache of 4 last recently used frames is kept to minimize wasted decompression for random
 * reads. The cache is only populated when seeking, so purely streaming access patterns are not
 * affected.
 *
 * NOTE: When a seek is performed, the entire frame it lands on will be decompressed and cached.
 * Usually this is desirable, but sometimes it can be worse than decompressing up to the requested
 * position and not caching, especially if the cache keeps getting thrashed. For that reason, the
 * cache will disable itself after 8 consecutive misses (num cached frames * 2).
 *
 * If the seek table is missing, corrupted, or incompatible, the stream is still usable, but
 * seeking will be slow.
 *
 * Seek tables with frame checksums are currently not supported.
 */

#define RWZSTD_SPAM 0
#define RWZSTD_STATS 0

#if RWZSTD_SPAM
#define SPAM(...) log_debug(__VA_ARGS__)
#else
#define SPAM(...) ((void)0)
#endif

typedef struct ZstdReaderSeekTableEntry {
	uint64_t comp_offset;
	uint64_t decomp_offset;
} ZstdReaderSeekTableEntry;

typedef struct ZstdReaderSeekTable {
	uint32_t num_entries;
	ZstdReaderSeekTableEntry entries[];
} ZstdReaderSeekTable;

typedef struct ZstdFrameCacheEntry {
	uint8_t *data;
	uint64_t lru_seq;
	uint32_t frame_idx;
	uint32_t filled_size;
	uint32_t alloc_size;
} ZstdFrameCacheEntry;

#define CACHE_FRAME_IDX_EMPTY UINT32_MAX

#define RWZSTD_NUM_CACHED_FRAMES 4
#define RWZSTD_MAX_CONSECUTIVE_CACHE_MISSES (RWZSTD_NUM_CACHED_FRAMES * 2)

typedef struct ZstdFrameCacheDrain {
	uint8_t *ptr;
	uint32_t remaining;
	uint8_t _end_marker[0];
} ZstdFrameCacheDrain;

typedef struct ZstdFrameCache {
	union {
		ZstdFrameCacheDrain drain;
		struct {
			char _padding[offsetof(ZstdFrameCacheDrain, _end_marker)];
			bool enabled;
			uint8_t consecutive_misses;
		};
	};

	uint64_t lru_seq;
	ZstdFrameCacheEntry entries[RWZSTD_NUM_CACHED_FRAMES];
} ZstdFrameCache;

typedef struct ZstdData {
	SDL_IOStream *wrapped;
	SDL_IOStream *iostream;
	int64_t pos;
	bool autoclose;

	union {
		struct {
			ZSTD_CStream *stream;
			ZSTD_outBuffer out_buffer;
		} writer;

		struct {
			ZSTD_DStream *stream;
			ZSTD_inBuffer in_buffer;
			size_t in_buffer_alloc_size;
			size_t next_read_size;
			int64_t uncompressed_size;
			ZstdReaderSeekTable *seek_table;
			ZstdFrameCache frame_cache;

			#if RWZSTD_STATS
			struct {
				size_t total_bytes_served;
				size_t total_bytes_served_from_cache;
				size_t total_bytes_decompressed;
			} stats;
			#endif
		} reader;
	};
} ZstdData;

#define ZSTD_SEEKTABLE_FRAME_MAGIC  0x184D2A5E
#define ZSTD_SEEKTABLE_FOOTER_MAGIC 0x8F92EAB1

#define ZSTD_SEEKTABLE_DESC_CHECKSUM_BIT  0b10000000
#define ZSTD_SEEKTABLE_DESC_RESERVED_BITS 0b01111100
#define ZSTD_SEEKTABLE_DESC_UNUSED_BITS   0b00000011

#define ZSTD_SEEKTABLE_DESC_INCOMPATIBLE_BITS \
	(ZSTD_SEEKTABLE_DESC_CHECKSUM_BIT | ZSTD_SEEKTABLE_DESC_RESERVED_BITS)

typedef struct __attribute__((packed)) ZstdFrameHeader {
	uint32_t magic;
	uint32_t frame_size;
} ZstdFrameHeader;

typedef struct __attribute__((packed)) ZstdSeekTableFooter {
	uint32_t num_frames;
	uint8_t descriptor;
	uint32_t magic;
} ZstdSeekTableFooter;

typedef struct __attribute__((packed)) ZstdSeekTableEntry {
	uint32_t comp_size;
	uint32_t decomp_size;
} ZstdSeekTableEntry;

static const ZstdReaderSeekTableEntry *rwzstd_find_seektable_entry(const ZstdReaderSeekTable *st, uint64_t offset) {
	assume(st->num_entries > 0);

	uint32_t lo = 0;
	uint32_t hi = st->num_entries - 1;

	if(offset >= st->entries[hi].decomp_offset) {
		return &st->entries[hi];
	}

	while(lo + 1 < hi) {
		uint32_t mid = lo + ((hi - lo) >> 1);

		if(st->entries[mid].decomp_offset <= offset) {
			lo = mid;
		} else {
			hi = mid;
		}
	}

	return &st->entries[lo];
}

static int64_t rwzstd_seek_disabled(void *ctx, int64_t offset, SDL_IOWhence whence) {
	ZstdData *zdata = ctx;

	if(offset == 0 && whence == SDL_IO_SEEK_CUR) {
		return zdata->pos;
	}

	return SDL_SetError("Can't seek in a zstd stream");
}

static int64_t rwzstd_size(void *ctx) {
	ZstdData *zdata = ctx;
	int64_t size = zdata->reader.uncompressed_size;

	if(size < 0) {
		SDL_SetError("Uncompressed stream size is unknown");
	}

	return size;
}

static bool rwzstd_reader_close(void *ctx);
static bool rwzstd_writer_close(void *ctx);

static bool rwzstd_close(void *ctx) {
	ZstdData *zdata = ctx;
	bool ok = true;

	if(zdata->autoclose) {
		ok = SDL_CloseIO(zdata->wrapped);
	}

	mem_free(zdata);
	return ok;
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

static int64_t rwzstd_reader_seek(void *ctx, int64_t offset, SDL_IOWhence whence);
static bool rwzstd_writer_flush(void *ctx, SDL_IOStatus *status);

static SDL_IOStream *rwzstd_alloc(ZstdData **out_zdata, SDL_IOStream *wrapped, bool autoclose, bool writer) {
	SDL_IOStreamInterface iface = { .version = sizeof(iface) };

	if(writer) {
		iface.close = rwzstd_writer_close;
		iface.flush = rwzstd_writer_flush;
		iface.read = rwzstd_read_invalid;
		iface.seek = rwzstd_seek_disabled;
		iface.write = rwzstd_write;
	} else {
		iface.close = rwzstd_reader_close;
		iface.read = rwzstd_read;
		iface.seek = rwzstd_reader_seek;
		iface.size = rwzstd_size;
		iface.write = rwzstd_write_invalid;
	}

	auto zdata = ALLOC(ZstdData, {
		.wrapped = wrapped,
		.autoclose = autoclose,
	});

	SDL_IOStream *io = SDL_OpenIO(&iface, zdata);

	if(!io) {
		mem_free(zdata);
	} else {
		auto props = SDL_GetIOProperties(io);
		SDL_SetPointerProperty(props, PROP_IOSTREAM_WRAPPED_STREAM, wrapped);

		zdata->iostream = io;
		*out_zdata = zdata;
	}

	return io;
}

static void rwzstd_reader_dump_stats(ZstdData *z) {
	#if RWZSTD_STATS
	log_debug("%s: ZSTD STATS:\n === Total served: %zu; from cache: %zu; total decompressed %zu; efficiency: %.02f%%; %% decompressed: %.02f%%; cache %sabled",
		iostream_get_name(z->iostream),
		z->reader.stats.total_bytes_served,
		z->reader.stats.total_bytes_served_from_cache,
		z->reader.stats.total_bytes_decompressed,
		100.0 * (double)z->reader.stats.total_bytes_served/(double)z->reader.stats.total_bytes_decompressed,
		100.0 * (double)z->reader.stats.total_bytes_decompressed/(double)z->reader.uncompressed_size,
		z->reader.frame_cache.enabled ? "en" : "dis"
	);
	#endif
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

		if(read == 0) {
			*status = SDL_GetIOStatus(z->wrapped);
			break;
		}

		assert(read <= end - pos);

		pos += read;
		in->size += read;
	}

	assert(in->size <= z->reader.in_buffer_alloc_size);
	return pos - start;
}

static ZstdFrameCacheEntry *rwzstd_cache_lookup(ZstdFrameCache *cache, uint32_t frame_idx) {
	for(int i = 0; i < ARRAY_SIZE(cache->entries); i++) {
		auto e = cache->entries + i;

		if(e->frame_idx == frame_idx) {
			e->lru_seq = ++cache->lru_seq;
			return e;
		}
	}

	return NULL;
}

static ZstdFrameCacheEntry *rwzstd_cache_eviction_slot(ZstdFrameCache *cache) {
	auto victim = cache->entries;

	for(int i = 0; i < ARRAY_SIZE(cache->entries); i++) {
		auto e = cache->entries + i;

		if(e->frame_idx == CACHE_FRAME_IDX_EMPTY) {
			return e;
		}

		if(e->lru_seq < victim->lru_seq) {
			victim = e;
		}
	}

	return victim;
}

static uint32_t rwzstd_seektable_entry_frame_size(
	const ZstdData *zdata, const ZstdReaderSeekTableEntry *e
) {
	auto st = NOT_NULL(zdata->reader.seek_table);

	uint32_t next_ofs =
		(e == &st->entries[st->num_entries - 1])
			? zdata->reader.uncompressed_size
			: e[1].decomp_offset;

	return next_ofs - e->decomp_offset;
}

/*
 * Decompress frame frame_idx into the cache.
 * The source stream must be positioned at the start of the frame.
 */
static ZstdFrameCacheEntry *rwzstd_cache_populate(ZstdData *z, uint32_t frame_idx) {
	ZstdFrameCache *cache = &z->reader.frame_cache;
	ZstdFrameCacheEntry *ce = rwzstd_cache_eviction_slot(cache);

	auto frame = z->reader.seek_table->entries + frame_idx;
	uint32_t frame_decomp_size = rwzstd_seektable_entry_frame_size(z, frame);

	if(ce->alloc_size < frame_decomp_size) {
		ce->data = mem_realloc(ce->data, frame_decomp_size);
		ce->alloc_size = frame_decomp_size;
	}

	ZSTD_outBuffer out = {
		.dst  = NOT_NULL(ce->data),
		.size = frame_decomp_size,
	};

	ZSTD_inBuffer *in = &z->reader.in_buffer;
	size_t hint = z->reader.next_read_size;
	SDL_IOStatus status = SDL_IO_STATUS_READY;

	while(hint != 0) {
		if(in->size - in->pos < hint) {
			rwzstd_reader_fill_in_buffer(z, hint, &status);

			if(in->size - in->pos < hint) {
				log_error("%s: Unexpected end of input while caching frame %u",
					iostream_get_name(z->iostream), frame_idx);
				return NULL;
			}
		}

		hint = ZSTD_decompressStream(z->reader.stream, &out, in);

		if(UNLIKELY(ZSTD_isError(hint))) {
			log_error("%s: ZSTD_decompressStream() failed while caching frame %u: %s",
				iostream_get_name(z->iostream), frame_idx, ZSTD_getErrorName(hint));
			return NULL;
		}
	}

	// hint == 0: frame boundary reached. Update next_read_size so the
	// normal read path can continue without a seek after cache drain.
	z->reader.next_read_size = hint;

	ce->frame_idx = frame_idx;
	ce->filled_size = (uint32_t)out.pos;
	ce->lru_seq = ++cache->lru_seq;

	assert(ce->filled_size == frame_decomp_size);

	#if RWZSTD_STATS
	z->reader.stats.total_bytes_decompressed += out.pos;
	#endif

	log_debug("%s: Cached frame %u (%u bytes decompressed)",
		iostream_get_name(z->iostream), frame_idx, ce->filled_size);

	return ce;
}

static int64_t rwzstd_reader_seek_source(ZstdData *z, int64_t comp_pos, SDL_IOWhence whence, int64_t decomp_pos) {
	int64_t srcpos = SDL_SeekIO(z->wrapped, comp_pos, whence);

	if(srcpos < 0) {
		log_sdl_error(LOG_ERROR, "SDL_SeekIO");
		return srcpos;
	}

	assert(srcpos == comp_pos || whence == SDL_IO_SEEK_END);

	z->pos = decomp_pos;
	z->reader.in_buffer.pos = 0;
	z->reader.in_buffer.size = 0;
	z->reader.next_read_size = ZSTD_initDStream(z->reader.stream);

	return decomp_pos;
}

static size_t rwzstd_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status) {
	ZstdData *z = ctx;
	ZstdFrameCache *cache = &z->reader.frame_cache;
	attr_unused size_t request_size = size;

	/*
	 * Drain any pending cache window first. This window is installed by
	 * rwzstd_reader_seek() after a cache hit or populate and covers the
	 * remainder of the cached frame starting at the seek target. Once
	 * exhausted the stream decompressor takes over from the next frame.
	 */
	size_t total_read = 0;
	size_t attr_unused from_cache = 0;

	if(cache->drain.remaining > 0) serve_cached: {
		size_t cached_read = min(size, cache->drain.remaining);
		memcpy(ptr, cache->drain.ptr, cached_read);
		cache->drain.ptr += cached_read;
		cache->drain.remaining -= (uint32_t)cached_read;
		z->pos += cached_read;
		size -= cached_read;
		ptr = (uint8_t*)ptr + cached_read;
		total_read += cached_read;
		from_cache += cached_read;
		#if RWZSTD_STATS
		z->reader.stats.total_bytes_served_from_cache += cached_read;
		#endif
	}

	if(size == 0) {
		goto end;
	}

	ZSTD_inBuffer *in = &z->reader.in_buffer;
	ZSTD_outBuffer out = {
		.dst  = ptr,
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
			*status = SDL_IO_STATUS_ERROR;
			SDL_SetError("ZSTD_decompressStream() failed: %s", ZSTD_getErrorName(zstatus));
			log_debug("%s", SDL_GetError());
			return 0;
		}

		z->reader.next_read_size = zstatus;

		if(zstatus == 0 && z->reader.frame_cache.enabled && out.pos < out.size) {
			// We are crossing a frame boundary.
			// See if we have the next frame cached so we can skip decompressing it.
			int64_t partial_read = out.pos;
			int64_t this_ofs = z->pos + partial_read;
			auto cache = &z->reader.frame_cache;
			auto st = NOT_NULL(z->reader.seek_table);

			for(uint i = 0; i < ARRAY_SIZE(cache->entries); ++i) {
				auto e = cache->entries + i;
				if(e->frame_idx == CACHE_FRAME_IDX_EMPTY) {
					continue;
				}

				auto st_e = st->entries + e->frame_idx;
				if(st_e->decomp_offset != this_ofs) {
					continue;
				}

				// Found it!
				// Set up the drain window and jump back.
				e->lru_seq = ++cache->lru_seq;
				cache->drain.ptr = e->data;
				cache->drain.remaining = e->filled_size;
				size -= partial_read;
				total_read += partial_read;
				ptr = (uint8_t*)out.dst + partial_read;
				z->pos = this_ofs;

				// Position the source stream at the next frame, so we can
				// start decoding it if we exhaust the drain window.

				auto st_e_next = st_e + 1;
				int64_t seek_status = -1;

				if(st_e_next < st->entries + st->num_entries) {
					seek_status = rwzstd_reader_seek_source(
						z, st_e_next->comp_offset, SDL_IO_SEEK_SET, this_ofs);
				} else {
					// This is the last frame… and we don't know where it ends.
					// That information exists in the seek table, so we could
					// have stored it, but seeking to the very end of the file
					// is probably also fine…
					seek_status = rwzstd_reader_seek_source(z, 0, SDL_IO_SEEK_END, this_ofs);
				}

				if(UNLIKELY(seek_status < 0)) {
					// … or maybe it wasn't. oh well.
					*status = SDL_IO_STATUS_ERROR;
					return 0;
				}

				goto serve_cached;  // considered harmful, my ass
			}
		}
	}

	total_read += out.pos;
	z->pos += out.pos;

end:
	SPAM("%s: READ(%zu): %zu from cache, %zu decompressed",
		iostream_get_name(z->iostream), request_size, from_cache, total_read - from_cache);

	#if RWZSTD_STATS
	z->reader.stats.total_bytes_served += total_read;
	z->reader.stats.total_bytes_decompressed += total_read - from_cache;
	#endif

	return total_read;
}

static bool rwzstd_reader_close(void *ctx) {
	ZstdData *z = ctx;

	if(z->reader.seek_table) {
		rwzstd_reader_dump_stats(z);
	}

	ZSTD_freeDStream(z->reader.stream);
	mem_free((void*)z->reader.in_buffer.src);
	mem_free(z->reader.seek_table);

	ZstdFrameCache *cache = &z->reader.frame_cache;
	for(int i = 0; i < ARRAY_SIZE(cache->entries); i++) {
		mem_free(cache->entries[i].data);
	}

	return rwzstd_close(z);
}

static bool rwzstd_reader_init_seektable(ZstdData *zdata) {
	SDL_IOStream *src = zdata->wrapped;
	int64_t init_ofs = SDL_TellIO(src);
	ZstdReaderSeekTable *st = NULL;

	if(init_ofs < 0) {
		goto fail;
	}

	ZstdSeekTableFooter footer;

	int64_t footer_ofs = SDL_SeekIO(src, -sizeof(footer), SDL_IO_SEEK_END);
	if(footer_ofs < 0) {
		log_sdl_error(LOG_ERROR, "SDL_SeekIO");
		goto fail;
	}

	int64_t r = SDL_ReadIO(src, &footer, sizeof(footer));
	if(r != sizeof(footer)) {
		if(r < 0) {
			log_sdl_error(LOG_ERROR, "SDL_ReadIO");
		} else {
			log_error("%s: SDL_ReadIO() returned %lli, expected %zi",
				iostream_get_name(src), (long long int)r, sizeof(footer));
		}
		goto fail;
	}

	footer.magic = SDL_Swap32LE(footer.magic);

	if(footer.magic != ZSTD_SEEKTABLE_FOOTER_MAGIC) {
		SPAM("%s: No seek table (bad magic %08x)", iostream_get_name(src), footer.magic);
		goto fail;
	}

	if(footer.descriptor & ZSTD_SEEKTABLE_DESC_INCOMPATIBLE_BITS) {
		log_debug("%s: Seek table ignored: Incompatible descriptor flags", iostream_get_name(src));
		goto fail;
	}

	footer.num_frames = SDL_Swap32LE(footer.num_frames);

	ZstdFrameHeader head;
	uint64_t expected_table_size = footer.num_frames * sizeof(ZstdSeekTableEntry);

	if(expected_table_size > UINT32_MAX - sizeof(head) - sizeof(footer)) {
		log_debug("%s: Seek table ignored: Too many frames or false match", iostream_get_name(src));
		goto fail;
	}

	if(footer.num_frames < 2) {
		log_debug("%s: Seek table ignored: Too few frames", iostream_get_name(src));
		goto fail;
	}

	int64_t seektable_ofs = SDL_SeekIO(src, footer_ofs - expected_table_size - sizeof(head), SDL_IO_SEEK_SET);
	if(seektable_ofs < 0) {
		log_sdl_error(LOG_ERROR, "SDL_SeekIO");
		goto fail;
	}

	r = SDL_ReadIO(src, &head, sizeof(head));
	if(r != sizeof(head)) {
		if(r < 0) {
			log_sdl_error(LOG_ERROR, "SDL_ReadIO");
		} else {
			log_error("%s: SDL_ReadIO() returned %lli, expected %zi",
				iostream_get_name(src), (long long int)r, sizeof(head));
		}
		goto fail;
	}

	head.magic = SDL_Swap32LE(head.magic);

	if(head.magic != ZSTD_SEEKTABLE_FRAME_MAGIC) {
		log_debug("%s: Seek table ignored: Bad frame header %08x", iostream_get_name(src), head.magic);
		goto fail;
	}

	head.frame_size = SDL_Swap32LE(head.frame_size);

	if(head.frame_size != expected_table_size + sizeof(footer)) {
		log_debug("%s: Seek table ignored: Unexpected frame size %u", iostream_get_name(src), head.frame_size);
		goto fail;
	}

	auto scratch = acquire_scratch_arena();
	ZstdSeekTableEntry *entries = marena_alloc_array(scratch, footer.num_frames, sizeof(*entries));

	r = SDL_ReadIO(src, entries, expected_table_size);
	if(r != expected_table_size) {
		if(r < 0) {
			log_sdl_error(LOG_ERROR, "SDL_ReadIO");
		} else {
			log_error("%s: SDL_ReadIO() returned %lli, expected %lli",
				iostream_get_name(src), (long long int)r, (long long int)expected_table_size);
		}
		release_scratch_arena(scratch);
		goto fail;
	}

	st = ALLOC_FLEX(ZstdReaderSeekTable, sizeof(*st->entries) * footer.num_frames);
	st->num_entries = 0;

	uint64_t comp_accum   = 0;
	uint64_t decomp_accum = 0;

	for(uint i = 0; i < footer.num_frames; ++i) {
		st->entries[st->num_entries++] = (ZstdReaderSeekTableEntry) {
			.comp_offset   = comp_accum,
			.decomp_offset = decomp_accum,
		};

		comp_accum   += SDL_Swap32LE(entries[i].comp_size);
		decomp_accum += SDL_Swap32LE(entries[i].decomp_size);
	}

	release_scratch_arena(scratch);

	if(zdata->reader.uncompressed_size < 0) {
		zdata->reader.uncompressed_size = decomp_accum;
	} else if(decomp_accum != zdata->reader.uncompressed_size) {
		log_warn("%s: Seek table ignored: Decompressed size mismatch: %llu != %llu",
			iostream_get_name(src), (long long int)decomp_accum, (long long int)zdata->reader.uncompressed_size);
		goto fail;
	}

	SDL_SeekIO(src, init_ofs, SDL_IO_SEEK_SET);
	zdata->reader.seek_table = st;

	/* Cache entry buffers are allocated lazily on first populate. */
	ZstdFrameCache *cache = &zdata->reader.frame_cache;
	for(int i = 0; i < ARRAY_SIZE(cache->entries); i++) {
		cache->entries[i].frame_idx = CACHE_FRAME_IDX_EMPTY;
		cache->entries[i].data = NULL;
	}

	log_debug("%s: Loaded seek table with %u entries", iostream_get_name(src), st->num_entries);

	zdata->reader.frame_cache.enabled = true;
	return true;

fail:
	mem_free(st);
	SDL_SeekIO(src, init_ofs, SDL_IO_SEEK_SET);
	return false;
}

static SDL_IOStream *rwzstd_open_reader(SDL_IOStream *src, bool autoclose, int64_t uncompressed_size) {
	if(!src) {
		return NULL;
	}

	ZstdData *z;
	SDL_IOStream *io = rwzstd_alloc(&z, src, autoclose, false);

	if(!io) {
		return NULL;
	}

	assume(z != NULL);

	z->reader.stream = NOT_NULL(ZSTD_createDStream());
	z->reader.next_read_size = ZSTD_initDStream(z->reader.stream);
	z->reader.in_buffer_alloc_size = max(z->reader.next_read_size, 16384);
	z->reader.in_buffer.src = mem_alloc(z->reader.in_buffer_alloc_size);
	z->reader.next_read_size = z->reader.in_buffer_alloc_size;
	z->reader.uncompressed_size = uncompressed_size;

	// Try to load the seekable format seek table if it exists.
	// If successful, this will also init uncompressed_size if it's unknown.
	rwzstd_reader_init_seektable(z);

	if(z->reader.uncompressed_size < 0) {
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
			log_error("%s: Error getting frame content size from zstd stream", iostream_get_name(io));
		} else if(fcs == ZSTD_CONTENTSIZE_UNKNOWN) {
			log_info("%s: zstd frame content size is unknown", iostream_get_name(io));
		} else if(fcs > INT64_MAX) {
			log_warn("%s: zstd frame content size is too large", iostream_get_name(io));
		} else {
			z->reader.uncompressed_size = fcs;
		}
	}

	return io;
}

static int rwzstd_reopen(void *ctx) {
	return rwzstd_reader_seek_source(ctx, 0, SDL_IO_SEEK_SET, 0) >= 0 ? 0 : -1;
}

static int64_t rwzstd_reader_seek_cached(ZstdData *zdata, int64_t target_pos) {
	auto st = NOT_NULL(zdata->reader.seek_table);
	auto cache = &zdata->reader.frame_cache;
	auto st_entry = rwzstd_find_seektable_entry(st, target_pos);
	uint32_t frame_idx = (uint32_t)(st_entry - st->entries);

	assert(cache->enabled);

	cache->drain.ptr = NULL;
	cache->drain.remaining = 0;

	ZstdFrameCacheEntry *ce = rwzstd_cache_lookup(cache, frame_idx);

	if(!ce) {
		SPAM("%s: Frame cache miss for frame %u; populating",
			iostream_get_name(zdata->iostream), frame_idx);

		assert(cache->consecutive_misses < RWZSTD_MAX_CONSECUTIVE_CACHE_MISSES);
		if(++cache->consecutive_misses == RWZSTD_MAX_CONSECUTIVE_CACHE_MISSES) {
			cache->enabled = false;
			log_debug("%s: %d consecutive cache misses, disabling the cache",
				iostream_get_name(zdata->iostream), cache->consecutive_misses);

			for(uint i = 0; i < ARRAY_SIZE(cache->entries); ++i) {
				auto e = cache->entries + i;
				mem_free(e->data);
				*e = (typeof(*e)) {
					.frame_idx = CACHE_FRAME_IDX_EMPTY,
				};
			}

			// recursive call, but will fallback to non-cached path now
			return rwzstd_reader_seek(zdata, target_pos, SDL_IO_SEEK_SET);
		}

		int64_t seek_status = rwzstd_reader_seek_source(
			zdata, st_entry->comp_offset, SDL_IO_SEEK_SET, st_entry->decomp_offset);

		if(UNLIKELY(seek_status < 0)) {
			return -1;
		}

		ce = rwzstd_cache_populate(zdata, frame_idx);

		if(!ce) {
			return SDL_SetError("Failed to populate frame cache");
		}

		// stream is now positioned at the start of the next frame
	} else {
		cache->consecutive_misses = 0;

		/*
		 * Cache hit: the stream is at an arbitrary position. Seek it to
		 * the start of the next frame so normal decompression continues
		 * seamlessly once the cache drain window is exhausted.
		 */
		uint32_t next_idx = frame_idx + 1;
		if(next_idx < st->num_entries) {
			int64_t seek_status = rwzstd_reader_seek_source(zdata,
				st->entries[next_idx].comp_offset, SDL_IO_SEEK_SET,
				st->entries[next_idx].decomp_offset);

			if(UNLIKELY(seek_status < 0)) {
				return -1;
			}
		}

		SPAM("%s: Frame cache hit: frame %u", iostream_get_name(zdata->iostream), frame_idx);
	}

	uint32_t local_pos = (uint32_t)(target_pos - st_entry->decomp_offset);
	cache->drain.ptr = ce->data + local_pos;
	cache->drain.remaining = ce->filled_size - local_pos;
	zdata->pos = target_pos;
	return target_pos;
}

static int64_t rwzstd_reader_seek(void *ctx, int64_t offset, SDL_IOWhence whence) {
	ZstdData *zdata = ctx;
	int64_t target_pos = rwutil_compute_seek_pos(offset, whence, zdata->pos, zdata->reader.uncompressed_size);
	attr_unused int64_t orig_pos = zdata->pos;

	if(UNLIKELY(target_pos < 0)) {
		return SDL_SetError("Can't seek in a zstd stream");
	}

	if(target_pos == zdata->pos) {
		return target_pos;
	}

	SPAM("%s: %zi --> %zi", iostream_get_name(zdata->iostream), zdata->pos, target_pos);

	auto st = zdata->reader.seek_table;
	int64_t skip_begin = 0;

	if(st) {
		if(zdata->reader.frame_cache.enabled) {
			return rwzstd_reader_seek_cached(zdata, target_pos);
		}

		auto st_e = rwzstd_find_seektable_entry(st, target_pos);
		if(target_pos < zdata->pos || zdata->pos < st_e->decomp_offset) {
			int64_t seek_result = rwzstd_reader_seek_source(
				zdata, st_e->comp_offset, SDL_IO_SEEK_SET, st_e->decomp_offset);

			if(UNLIKELY(seek_result < 0)) {
				return seek_result;
			}

			skip_begin = st_e->decomp_offset;
		}
	}

	char skip_buf[1024];

	if(skip_begin == 0 && target_pos >= zdata->pos) {
		skip_begin = zdata->pos;
	}

	int64_t skip_result = rwutil_seek_emulated_abs(
		zdata->iostream, target_pos, &zdata->pos, rwzstd_reopen, zdata, sizeof(skip_buf), skip_buf);

	if(UNLIKELY(skip_result < 0)) {
		return skip_result;
	}

	assert(skip_result == zdata->pos);
	assert(skip_result == target_pos);

	int64_t attr_unused waste = skip_result - skip_begin;
	SPAM("Seek %zi --> %zi; skip waste: %zi", orig_pos, target_pos, waste);

	#if RWZSTD_STATS
	assert(zdata->reader.stats.total_bytes_served > waste);
	zdata->reader.stats.total_bytes_served -= waste;
	#endif

	return skip_result;
}

static bool rwzstd_compress(
	ZstdData *z, ZSTD_inBuffer *in, ZSTD_EndDirective edir, size_t *zstatus, SDL_IOStatus *io_status
) {
	ZSTD_outBuffer *out = &z->writer.out_buffer;
	*zstatus = ZSTD_compressStream2(z->writer.stream, out, in, edir);

	if(UNLIKELY(ZSTD_isError(*zstatus))) {
		SDL_SetError("ZSTD_compressStream2() failed: %s", ZSTD_getErrorName(*zstatus));
		log_debug("%s", SDL_GetError());
		*io_status = SDL_IO_STATUS_ERROR;
		return false;
	}

	if(out->pos > 0) {
		size_t written = SDL_WriteIO(z->wrapped, out->dst, out->pos);

		if(UNLIKELY(written != out->pos)) {
			SDL_SetError("SDL_WriteIO() returned %zi; expected %zi. Error: %s", written, out->pos, SDL_GetError());
			log_debug("%s", SDL_GetError());
			*io_status = SDL_GetIOStatus(z->wrapped);
			return false;
		}

		out->pos = 0;
	}

	return true;
}

SDL_IOStream *SDL_RWWrapZstdReader(SDL_IOStream *src, int64_t uncompressed_size, bool autoclose) {
	return rwzstd_open_reader(src, autoclose, uncompressed_size);
}

static size_t rwzstd_write(void *ctx, const void *data_in, size_t size, SDL_IOStatus *status) {
	ZstdData *z = ctx;
	size_t zstatus;

	if(UNLIKELY(size == 0)) {
		return 0;
	}

	ZSTD_inBuffer in = {
		.src = data_in,
		.size = size,
	};

	while(in.pos < in.size) {
		if(UNLIKELY(!rwzstd_compress(z, &in, ZSTD_e_continue, &zstatus, status))) {
			return 0;
		}
	}

	z->pos += size;
	return size;
}

static bool rwzstd_writer_flush(void *ctx, SDL_IOStatus *status) {
	ZstdData *z = ctx;
	size_t zstatus;

	do {
		if(UNLIKELY(!rwzstd_compress(z, &(ZSTD_inBuffer){}, ZSTD_e_end, &zstatus, status))) {
			return false;
		}
	} while(zstatus != 0);

	return true;
}

static bool rwzstd_writer_close(void *ctx) {
	ZstdData *z = ctx;
	SDL_IOStatus status;
	bool flush_ok = rwzstd_writer_flush(z, &status);

	ZSTD_freeCStream(z->writer.stream);
	mem_free(z->writer.out_buffer.dst);

	bool r = rwzstd_close(z);
	return r && flush_ok;
}

SDL_IOStream *SDL_RWWrapZstdWriter(SDL_IOStream *src, int clevel, bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	ZstdData *z;
	SDL_IOStream *io = rwzstd_alloc(&z, src, autoclose, true);

	if(UNLIKELY(!io)) {
		return NULL;
	}

	if(UNLIKELY(clevel < ZSTD_minCLevel() || clevel > ZSTD_maxCLevel())) {
		log_warn("Invalid compression level %i", clevel);
		clevel = RW_ZSTD_LEVEL_DEFAULT;
		assert(0);
	}

	z->writer.stream = NOT_NULL(ZSTD_createCStream());
	ZSTD_initCStream(z->writer.stream, clevel);
	z->writer.out_buffer.size = ZSTD_CStreamOutSize();
	z->writer.out_buffer.dst = mem_alloc(z->writer.out_buffer.size);

	return io;
}
