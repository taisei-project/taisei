
#ifndef TSUTIL_H
#define TSUTIL_H

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h> // compiling under mingw may fail without this...

//
// string utils
//

#undef strlcat
#undef strlcpy
#define strlcat SDL_strlcat
#define strlcpy SDL_strlcpy

#undef strncat
#undef strncpy
#define strncat DO_NOT_USE_strncat_USE_strlcat
#define strncpy DO_NOT_USE_strncpy_USE_strlcpy

char* copy_segment(const char *text, const char *delim, int *size);
bool strendswith(const char *s, const char *e);
bool strstartswith(const char *s, const char *p);
bool strendswith_any(const char *s, const char **earray);
void stralloc(char **dest, const char *src);
char* strjoin(const char *first, ...);
char* strfmt(const char *fmt, ...);
#undef strdup
#define strdup SDL_strdup

//
// math utils
//

#include <complex.h>

// These definitions are common but non-standard, so we provide our own
#undef M_PI
#undef M_PI_2
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923

// This is a workaround to properly specify the type of our "complex" variables...
// Taisei code always uses just "complex" when it actually means "complex double", which is not really correct...
// gcc doesn't seem to care, other compilers do (e.g. clang)
#ifdef complex
    #undef complex
    #define complex _Complex double
#endif

// needed for mingw compatibility:
#undef min
#undef max

double min(double, double);
double max(double, double);
double clamp(double, double, double);
double approach(double v, double t, double d);
double psin(double);

#define SIGN(x) ((x > 0) - (x < 0))

//
// gl/video utils
//

typedef struct {
    int fpstime;  // frame counter
    int fps;
    int show_fps;
    double stagebg_fps;
} FPSCounter;

void frame_rate(int *lasttime);
void calc_fps(FPSCounter *fps);
void set_ortho(void);
void colorfill(float r, float g, float b, float a);
void fade_out(float f);

//
// i/o utils
//

char* read_all(const char *filename, int *size);

//
// misc utils
//

int getenvint(const char *v);

#endif
