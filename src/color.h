/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#ifndef TSCOLOR_H
#define TSCOLOR_H

typedef struct {
    float r;
    float g;
    float b;
    float a;
} Color;

Color* rgba(float r, float g, float b, float a);
Color* rgb(float r, float g, float b);

#endif
