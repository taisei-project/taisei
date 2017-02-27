/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include <stdlib.h>
#include "color.h"

Color *rgba(float r, float g, float b, float a) {
    Color *clr = malloc(sizeof(Color));
    clr->r = r;
    clr->g = g;
    clr->b = b;
    clr->a = a;

    return clr;
}

Color *rgb(float r, float g, float b) {
    return rgba(r, g, b, 1.0);
}

