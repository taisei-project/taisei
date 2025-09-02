/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_segment.h"
#include "log.h"
#include "util/miscmath.h"

typedef struct Segment {
	SDL_IOStream *wrapped;
	size_t start;
	size_t end;
	int64_t pos; // fallback for non-seekable streams
	bool autoclose;
} Segment;

static int64_t segment_seek(void *ctx, int64_t offset, SDL_IOWhence whence) {
	Segment *s = ctx;

	switch(whence) {
		case SDL_IO_SEEK_CUR : {
			if(offset) {
				int64_t pos = SDL_TellIO(s->wrapped);

				if(pos < 0) {
					return pos;
				}

				if(pos + offset > s->end) {
					offset = s->end - pos;
				} else if(pos + offset < s->start) {
					offset = s->start - pos;
				}
			}

			break;
		}

		case SDL_IO_SEEK_SET : {
			offset += s->start;

			if(offset > s->end) {
				offset = s->end;
			}

			break;
		}

		case SDL_IO_SEEK_END : {
			int64_t size = SDL_GetIOSize(s->wrapped);

			if(size < 0) {
				return size;
			}

			if(size > s->end) {
				offset -= size - s->end;
			}

			if(size + offset < s->start) {
				offset += s->start - (size + offset);
			}

			break;
		}

		default: {
			SDL_SetError("Bad whence value %i", whence);
			return -1;
		}
	}

	int64_t result = SDL_SeekIO(s->wrapped, offset, whence);

	if(result > 0) {
		if(s->start > result) {
			result = 0;
		} else {
			result -= s->start;
		}
	}

	return result;
}

static int64_t segment_size(void *ctx) {
	Segment *s = ctx;
	int64_t size = SDL_GetIOSize(s->wrapped);

	if(size < 0) {
		return size;
	}

	if(size > s->end) {
		size = s->end;
	}

	return size - s->start;
}

static size_t segment_readwrite(
	Segment *s, void *ptr, size_t size, bool write, SDL_IOStatus *status
) {
	int64_t pos = SDL_TellIO(s->wrapped);

	if(pos < 0) {
		log_debug("SDL_TellIO failed (%i): %s", (int)pos, SDL_GetError());
		SDL_SetError("segment_readwrite: SDL_TellIO failed (%i) %s", (int)pos, SDL_GetError());

		// this could be a non-seekable stream, like /dev/stdin...
		// let's assume nothing else uses the wrapped stream and try to guess the current position
		// this only works if the actual position in the stream at the time of segment creation matched s->start...
		pos = s->pos;
	} else {
		s->pos = pos;
	}

	if(pos < s->start || pos > s->end) {
		log_warn("Segment range violation");
		SDL_SetError("segment_readwrite: segment range violation");
		return 0;
	}

	int64_t maxsize = s->end - pos;
	size = min(size, maxsize);

	if(write) {
		size = SDL_WriteIO(s->wrapped, ptr, size);
	} else {
		size = SDL_ReadIO(s->wrapped, ptr, size);
	}

	*status = SDL_GetIOStatus(s->wrapped);
	s->pos += size;
	assert(s->pos <= s->end);
	return size;
}

static size_t segment_read(void *ctx, void *ptr, size_t size, SDL_IOStatus *status) {
	return segment_readwrite(ctx, ptr, size, false, status);
}

static size_t segment_write(void *ctx, const void *ptr, size_t size, SDL_IOStatus *status) {
	return segment_readwrite(ctx, (void*)ptr, size, true, status);
}

static bool segment_close(void *ctx) {
	Segment *s = ctx;

	bool result = true;

	if(s->autoclose) {
		result = SDL_CloseIO(s->wrapped);
	}

	mem_free(s);
	return result;
}

SDL_IOStream *SDL_RWWrapSegment(SDL_IOStream *src, size_t start, size_t end, bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	assert(end > start);

	SDL_IOStreamInterface iface = {
		.version = sizeof(iface),
		.seek = segment_seek,
		.size = segment_size,
		.read = segment_read,
		.write = segment_write,
		.close = segment_close,
	};

	auto s = ALLOC(Segment, {
		.wrapped = src,
		.start = start,
		.end = end,
		.autoclose = autoclose,
		.pos = start,  // fallback for non-seekable streams
	});

	SDL_IOStream *io = SDL_OpenIO(&iface, s);

	if(UNLIKELY(!io)) {
		mem_free(s);
		return NULL;
	}

	return io;
}
