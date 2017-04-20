/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include <stdio.h>
#include "color.h"

static const float conv = 1.0f / CLR_ONEVALUE;

Color rgba(float r, float g, float b, float a) {
    return ((((Color)(CLR_ONEVALUE * (r)) & CLR_CMASK) << CLR_R) + \
            (((Color)(CLR_ONEVALUE * (g)) & CLR_CMASK) << CLR_G) + \
            (((Color)(CLR_ONEVALUE * (b)) & CLR_CMASK) << CLR_B) + \
            (((Color)(CLR_ONEVALUE * (a)) & CLR_CMASK) << CLR_A));
}

Color rgb(float r, float g, float b) {
    return ((((Color)(CLR_ONEVALUE * (r)) & CLR_CMASK) << CLR_R) + \
            (((Color)(CLR_ONEVALUE * (g)) & CLR_CMASK) << CLR_G) + \
            (((Color)(CLR_ONEVALUE * (b)) & CLR_CMASK) << CLR_B) + \
            (CLR_ONEVALUE << CLR_A));
}

void parse_color(Color clr, float *r, float *g, float *b, float *a) {
    *r = (ColorComponent)((clr >> CLR_R) & CLR_CMASK) * conv;
    *g = (ColorComponent)((clr >> CLR_G) & CLR_CMASK) * conv;
    *b = (ColorComponent)((clr >> CLR_B) & CLR_CMASK) * conv;
    *a = (ColorComponent)((clr >> CLR_A) & CLR_CMASK) * conv;
}

void parse_color_call(Color clr, tsglColor4f_ptr func) {
    float r, g, b, a;
    parse_color(clr, &r, &g, &b, &a);
    func(r, g, b, a);
}

void parse_color_array(Color clr, float array[4]) {
    parse_color(clr, array, array+1, array+2, array+3);
}

Color derive_color(Color src, Color mask, Color mod) {
    return (src & ~mask) | (mod & mask);
}

float color_component(Color clr, unsigned int ofs) {
    return (ColorComponent)((clr >> ofs) & CLR_CMASK) * conv;
}

Color multiply_colors(Color c1, Color c2) {
    float c1a[4], c2a[4];
    parse_color_array(c1, c1a);
    parse_color_array(c2, c2a);
    return rgba(c1a[0]*c2a[0], c1a[1]*c2a[1], c1a[2]*c2a[2], c1a[3]*c2a[3]);
}

Color add_colors(Color c1, Color c2) {
    float c1a[4], c2a[4];
    parse_color_array(c1, c1a);
    parse_color_array(c2, c2a);
    return rgba(c1a[0]+c2a[0], c1a[1]+c2a[1], c1a[2]+c2a[2], c1a[3]+c2a[3]);
}

Color subtract_colors(Color c1, Color c2) {
    float c1a[4], c2a[4];
    parse_color_array(c1, c1a);
    parse_color_array(c2, c2a);
    return rgba(c1a[0]-c2a[0], c1a[1]-c2a[1], c1a[2]-c2a[2], c1a[3]-c2a[3]);
}

Color divide_colors(Color c1, Color c2) {
    float c1a[4], c2a[4];
    parse_color_array(c1, c1a);
    parse_color_array(c2, c2a);
    return rgba(c1a[0]/c2a[0], c1a[1]/c2a[1], c1a[2]/c2a[2], c1a[3]/c2a[3]);
}

Color mix_colors(Color c1, Color c2, double a) {
    float c1a[4], c2a[4];
    double f1 = a;
    double f2 = 1 - f1;
    parse_color_array(c1, c1a);
    parse_color_array(c2, c2a);
    return rgba(f1*c1a[0]+f2*c2a[0], f1*c1a[1]+f2*c2a[1], f1*c1a[2]+f2*c2a[2], f1*c1a[3]+f2*c2a[3]);
}

Color approach_color(Color src, Color dst, double delta) {
    float c1a[4], c2a[4];
    parse_color_array(src, c1a);
    parse_color_array(dst, c2a);
    return rgba(
        c1a[0] + (c2a[0] - c1a[0]) * delta,
        c1a[1] + (c2a[1] - c1a[1]) * delta,
        c1a[2] + (c2a[2] - c1a[2]) * delta,
        c1a[3] + (c2a[3] - c1a[3]) * delta
    );
}

// #define COLOR_TEST

int color_test(void) {
#ifdef COLOR_TEST
    float clra[4];
    Color clr1, clr2, clr3;

    clr1 = rgba(0.1, 0.2, 0.3, 0.4);
    parse_color_array(clr1, clra);
    clr2 = rgba(clra[0], clra[1], clra[2], clra[3]);

    clr3 = derive_color(clr1, CLRMASK_A, rgba(0, 0, 0, -1.0));
    printf("1: %016"PRIxMAX" (%f %f %f %f)\n", (uintmax_t)clr1,
        color_component(clr1, CLR_R), color_component(clr1, CLR_G), color_component(clr1, CLR_B), color_component(clr1, CLR_A));
    printf("2: %016"PRIxMAX" (%f %f %f %f)\n", (uintmax_t)clr2,
        color_component(clr2, CLR_R), color_component(clr2, CLR_G), color_component(clr2, CLR_B), color_component(clr2, CLR_A));
    printf("3: %016"PRIxMAX" (%f %f %f %f)\n", (uintmax_t)clr3,
        color_component(clr3, CLR_R), color_component(clr3, CLR_G), color_component(clr3, CLR_B), color_component(clr3, CLR_A));

    assert(clr1 == clr2);
    assert(color_component(clr3, CLR_R) == color_component(clr3, CLR_R));
    assert(color_component(clr3, CLR_G) == color_component(clr3, CLR_G));
    assert(color_component(clr3, CLR_B) == color_component(clr3, CLR_B));
    assert(color_component(clr3, CLR_A) == -1.0);
    return 1;
#else
    return 0;
#endif
}
