/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <stdint.h>
#include "taiseigl.h"
#include "util.h"

#define CLR_R ((Color)48)
#define CLR_G ((Color)32)
#define CLR_B ((Color)16)
#define CLR_A ((Color)0)

#define CLR_CMASK ((Color)0xffff)
#define CLR_ONEVALUE ((Color)0xff)

#define CLRMASK_R (CLR_CMASK << CLR_R)
#define CLRMASK_G (CLR_CMASK << CLR_G)
#define CLRMASK_B (CLR_CMASK << CLR_B)
#define CLRMASK_A (CLR_CMASK << CLR_A)

typedef uint64_t Color;
typedef int16_t ColorComponent;

#ifndef COLOR_INLINE
    #ifdef NDEBUG
        #define COLOR_INLINE
    #endif
#endif

#ifdef COLOR_INLINE
#define rgba(r,g,b,a) RGBA(r,g,b,a)
#define rgb(r,g,b) RGB(r,g,b)
#else
Color rgba(float r, float g, float b, float a) __attribute__((const));
Color rgb(float r, float g, float b) __attribute__((const));
#endif

Color hsla(float h, float s, float l, float a) __attribute__((const));
Color hsl(float h, float s, float l) __attribute__((const));

void parse_color(Color clr, float *r, float *g, float *b, float *a);
void parse_color_call(Color clr, tsglColor4f_ptr func);
void parse_color_array(Color clr, float array[4]);
Color derive_color(Color src, Color mask, Color mod) __attribute__((const));
Color multiply_colors(Color c1, Color c2) __attribute__((const));
Color add_colors(Color c1, Color c2) __attribute__((const));
Color subtract_colors(Color c1, Color c2) __attribute__((const));
Color mix_colors(Color c1, Color c2, double a) __attribute__((const));
Color approach_color(Color src, Color dst, double delta) __attribute__((const));
float color_component(Color clr, unsigned int ofs) __attribute__((const));
char* color_str(Color c);

#ifdef RGBA
#undef RGBA
#endif

#ifdef RGB
#undef RGB
#endif

#define RGBA(r,g,b,a) \
   ((((Color)(ColorComponent)(CLR_ONEVALUE * (r)) & CLR_CMASK) << CLR_R) + \
    (((Color)(ColorComponent)(CLR_ONEVALUE * (g)) & CLR_CMASK) << CLR_G) + \
    (((Color)(ColorComponent)(CLR_ONEVALUE * (b)) & CLR_CMASK) << CLR_B) + \
    (((Color)(ColorComponent)(CLR_ONEVALUE * (a)) & CLR_CMASK) << CLR_A))

#define RGB(r,g,b) \
   ((((Color)(ColorComponent)(CLR_ONEVALUE * (r)) & CLR_CMASK) << CLR_R) + \
    (((Color)(ColorComponent)(CLR_ONEVALUE * (g)) & CLR_CMASK) << CLR_G) + \
    (((Color)(ColorComponent)(CLR_ONEVALUE * (b)) & CLR_CMASK) << CLR_B) + \
    (CLR_ONEVALUE << CLR_A))

int color_test(void);
