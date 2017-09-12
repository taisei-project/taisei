/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "rwops_zipfile.h"
#include "util.h"

static int ziprw_close(SDL_RWops *rw) {
    if(rw) {
        if(rw->hidden.unknown.data2) {
            zip_fclose(rw->hidden.unknown.data1);
        }

        SDL_FreeRW(rw);
    }

    return 0;
}

static int64_t ziprw_seek(SDL_RWops *rw, int64_t offset, int whence) {
    SDL_SetError("Not implemented");
    return -1;
}

static int64_t ziprw_size(SDL_RWops *rw) {
    SDL_SetError("Not implemented");
    return -1;
}

static size_t ziprw_read(SDL_RWops *rw, void *ptr, size_t size, size_t maxnum) {
    zip_file_t *zipfile = rw->hidden.unknown.data1;
    // XXX: possible size overflow
    return zip_fread(zipfile, ptr, size * maxnum);
}

static size_t ziprw_write(SDL_RWops *rw, const void *ptr, size_t size, size_t maxnum) {
    SDL_SetError("Not implemented");
    return -1;
}

SDL_RWops* SDL_RWFromZipFile(zip_file_t *zipfile, bool autoclose) {
    SDL_RWops *rw = SDL_AllocRW();
    memset(rw, 0, sizeof(SDL_RWops));

    rw->hidden.unknown.data1 = zipfile;
    rw->hidden.unknown.data2 = (void*)(intptr_t)autoclose;
    rw->type = SDL_RWOPS_UNKNOWN;

    rw->size = ziprw_size;
    rw->seek = ziprw_seek;
    rw->close = ziprw_close;
    rw->read = ziprw_read;
    rw->write = ziprw_write;

    return rw;
}
