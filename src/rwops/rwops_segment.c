/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_segment.h"
#include "log.h"

typedef struct Segment {
	SDL_RWops *wrapped;
	size_t start;
	size_t end;
	int64_t pos; // fallback for non-seekable streams
	bool autoclose;
} Segment;

#define SEGMENT(rw) ((Segment*)((rw)->hidden.unknown.data1))

static int64_t segment_seek(SDL_RWops *rw, int64_t offset, int whence) {
	Segment *s = SEGMENT(rw);

	switch(whence) {
		case RW_SEEK_CUR: {
			if(offset) {
				int64_t pos = SDL_RWtell(s->wrapped);

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

		case RW_SEEK_SET: {
			offset += s->start;

			if(offset > s->end) {
				offset = s->end;
			}

			break;
		}

		case RW_SEEK_END: {
			int64_t size = SDL_RWsize(s->wrapped);

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

	int64_t result = SDL_RWseek(s->wrapped, offset, whence);

	if(result > 0) {
		if(s->start > result) {
			result = 0;
		} else {
			result -= s->start;
		}
	}

	return result;
}

static int64_t segment_size(SDL_RWops *rw) {
	Segment *s = SEGMENT(rw);
	int64_t size = SDL_RWsize(s->wrapped);

	if(size < 0) {
		return size;
	}

	if(size > s->end) {
		size = s->end;
	}

	return size - s->start;
}

static size_t segment_readwrite(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum, bool write) {
	Segment *s = SEGMENT(rw);
	int64_t pos = SDL_RWtell(s->wrapped);
	size_t onum;

	if(pos < 0) {
		log_debug("SDL_RWtell failed (%i): %s", (int)pos, SDL_GetError());
		SDL_SetError("segment_readwrite: SDL_RWtell failed (%i) %s", (int)pos, SDL_GetError());

		// this could be a non-seekable stream, like /dev/stdin...
		// let's assume nothing else uses the wrapped stream and try to guess the current position
		// this only works if the actual positon in the stream at the time of segment creation matched s->start...
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

	while(size * maxnum > maxsize) {
		if(!--maxnum) {
			return 0;
		}
	}

	if(write) {
		onum = SDL_RWwrite(s->wrapped, ptr, size, maxnum);
	} else {
		onum = SDL_RWread(s->wrapped, ptr, size, maxnum);
	}

	s->pos += onum / size;
	assert(s->pos <= s->end);

	return onum;
}

static size_t segment_read(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum) {
	return segment_readwrite(rw, ptr, size, maxnum, false);
}

static size_t segment_write(SDL_RWops *rw, const void *ptr, size_t size, size_t maxnum) {
	return segment_readwrite(rw, (void*)ptr, size, maxnum, true);
}

static int segment_close(SDL_RWops *rw) {
	if(rw) {
		Segment *s = SEGMENT(rw);

		if(s->autoclose) {
			SDL_RWclose(s->wrapped);
		}

		mem_free(s);
		SDL_FreeRW(rw);
	}

	return 0;
}

SDL_RWops *SDL_RWWrapSegment(SDL_RWops *src, size_t start, size_t end, bool autoclose) {
	if(UNLIKELY(!src)) {
		return NULL;
	}

	assert(end > start);

	SDL_RWops *rw = SDL_AllocRW();

	if(UNLIKELY(!rw)) {
		return NULL;
	}

	memset(rw, 0, sizeof(SDL_RWops));

	rw->type = SDL_RWOPS_UNKNOWN;
	rw->seek = segment_seek;
	rw->size = segment_size;
	rw->read = segment_read;
	rw->write = segment_write;
	rw->close = segment_close;
	rw->hidden.unknown.data1 = ALLOC(Segment, {
		.wrapped = src,
		.start = start,
		.end = end,
		.autoclose = autoclose,

		// fallback for non-seekable streams
		.pos = start,
	});

	return rw;
}
