/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif

#include "rwops_pipe.h"
#include "util.h"

#include <stdio.h>
#include <errno.h>

static int pipe_close(SDL_RWops *rw) {
    int status = 0;

    if(rw->hidden.stdio.autoclose) {
        if((status = pclose(rw->hidden.stdio.fp)) != 0) {
            SDL_SetError("Process exited abnormally: code %i", status);
            status = -1;
        }
    }

    SDL_FreeRW(rw);
    return status;
}

SDL_RWops* SDL_RWpopen(const char *command, const char *mode) {
    assert(command != NULL);
    assert(mode != NULL);

    FILE *fp = popen(command, mode);

    if(!fp) {
        SDL_SetError("popen(\"%s\", \"%s\") failed: errno %i", command, mode, errno);
        return NULL;
    }

    SDL_RWops *rw = SDL_RWFromFP(fp, true);

    if(!rw) {
        pclose(fp);
        return NULL;
    }

    if(SDL_RWConvertToPipe(rw)) {
        SDL_RWclose(rw);
        return NULL;
    }

    return rw;
}

int SDL_RWConvertToPipe(SDL_RWops *rw) {
    if(rw->type != SDL_RWOPS_STDFILE) {
        SDL_SetError("Not an stdio stream");
        return -1;
    }

    rw->close = pipe_close;
    return 0;
}
