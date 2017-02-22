
#include <assert.h>
#include <stdbool.h>
#include "rwops_segment.h"

typedef struct Segment {
    SDL_RWops *wrapped;
    size_t start;
    size_t end;
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
    int64_t size = SDL_RWsize(rw);

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

    if(pos < 0) {
        printf("segment_readwrite: SDL_RWtell failed (%i)", (int)pos);
        SDL_SetError("segment_readwrite: SDL_RWtell failed (%i)", (int)pos);
        return 0;
    }

    if(pos < s->start || pos > s->end) {
        printf("segment_readwrite: segment range violation");
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
        return SDL_RWwrite(s->wrapped, ptr, size, maxnum);
    } else {
        return SDL_RWread(s->wrapped, ptr, size, maxnum);
    }
}

static size_t segment_read(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum) {
    return segment_readwrite(rw, ptr, size, maxnum, false);
}

static size_t segment_write(SDL_RWops *rw, const void *ptr, size_t size, size_t maxnum) {
    return segment_readwrite(rw, (void*)ptr, size, maxnum, true);
}

static int segment_close(SDL_RWops *rw) {
    if(rw) {
        free(SEGMENT(rw));
        SDL_FreeRW(rw);
    }

    return 0;
}

SDL_RWops* SDL_RWWrapSegment(SDL_RWops *src, size_t start, size_t end) {
    assert(end > start);

    SDL_RWops *rw = SDL_AllocRW();

    if(!rw) {
        return NULL;
    }

    memset(rw, 0, sizeof(SDL_RWops));

    rw->type = SDL_RWOPS_UNKNOWN;
    rw->seek = segment_seek;
    rw->size = segment_size;
    rw->read = segment_read;
    rw->write = segment_write;
    rw->close = segment_close;

    Segment *s = malloc(sizeof(Segment));
    s->wrapped = src;
    s->start = start;
    s->end = end;

    rw->hidden.unknown.data1 = s;

    return rw;
}
