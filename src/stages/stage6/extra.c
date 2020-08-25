/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "common_tasks.h"

cmplx wrap_around(cmplx *pos) {
    // This function only works approximately. If more than one of these conditions are true,
    // dir has to correspond to the wall that was passed first for the
    // current preview display to work.
    //
    // with some clever geometry this could be either fixed here or in the
    // preview calculation, but the spell as it is currently seems to work
    // perfectly with this simplified version.

    cmplx dir = 0;
    if(creal(*pos) < -10) {
        *pos += VIEWPORT_W;
        dir += -1;
    }
    if(creal(*pos) > VIEWPORT_W+10) {
        *pos -= VIEWPORT_W;
        dir += 1;
    }
    if(cimag(*pos) < -10) {
        *pos += I*VIEWPORT_H;
        dir += -I;
    }
    if(cimag(*pos) > VIEWPORT_H+10) {
        *pos -= I*VIEWPORT_H;
        dir += I;
    }

    return dir;
}
