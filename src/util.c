
#include "util.h"
#include "global.h"

//
// string utils
//

bool strendswith(const char *s, const char *e) {
    int ls = strlen(s);
    int le = strlen(e);

    if(le > ls)
        return false;

    int i; for(i = 0; i < le; ++i)
        if(s[ls-i-1] != e[le-i-1])
            return false;

    return true;
}

bool strstartswith(const char *s, const char *p) {
    int ls = strlen(s);
    int lp = strlen(p);

    if(ls < lp)
        return false;

    return !strncmp(s, p, lp);
}

bool strendswith_any(const char *s, const char **earray) {
    for(const char **e = earray; *e; ++e) {
        if(strendswith(s, *e)) {
            return true;
        }
    }

    return false;
}

void stralloc(char **dest, const char *src) {
    free(*dest);

    if(src) {
        *dest = malloc(strlen(src)+1);
        strcpy(*dest, src);
    } else {
        *dest = NULL;
    }
}

char* strjoin(const char *first, ...) {
    va_list args;
    size_t size = strlen(first) + 1;
    char *str = malloc(size);

    strcpy(str, first);
    va_start(args, first);

    for(;;) {
        char *next = va_arg(args, char*);

        if(!next) {
            break;
        }

        size += strlen(next);
        str = realloc(str, size);
        strcat(str, next);
    }

    va_end(args);
    return str;
}

char* strfmt(const char *fmt, ...) {
    size_t written = 0;
    size_t fmtlen = strlen(fmt);
    size_t asize = 1;
    char *out = NULL;

    while(asize * 2 <= fmtlen)
        asize *= 2;

    do {
        asize *= 2;
        out = realloc(out, asize);
        va_list args;

        va_start(args, fmt);
        written = vsnprintf(out, asize, fmt, args);
        va_end(args);

    } while(written >= asize);

    return realloc(out, strlen(out) + 1);
}

char* copy_segment(const char *text, const char *delim, int *size) {
    char *seg, *beg, *end;

    beg = strstr(text, delim);
    if(!beg)
        return NULL;
    beg += strlen(delim);

    end = strstr(beg, "%%");
    if(!end)
        return NULL;

    *size = end-beg;
    seg = malloc(*size+1);
    strlcpy(seg, beg, *size+1);

    return seg;
}

void strip_trailing_slashes(char *buf) {
    for(char *c = buf + strlen(buf) - 1; c >= buf && (*c == '/' || *c == '\\'); c--)
        *c = 0;
}

//
// math utils
//

double approach(double v, double t, double d) {
    if(v < t) {
        v += d;
        if(v > t)
            return t;
    } else if(v > t) {
        v -= d;
        if(v < t)
            return t;
    }

    return v;
}

double psin(double x) {
    return 0.5 + 0.5 * sin(x);
}

double max(double a, double b) {
    return (a > b)? a : b;
}

double min(double a, double b) {
    return (a < b)? a : b;
}

double clamp(double f, double lower, double upper) {
    if(f < lower)
        return lower;
    if(f > upper)
        return upper;

    return f;
}

//
// gl/video utils
//

void frame_rate(int *lasttime) {
    if(global.frameskip) {
        return;
    }

    int t = *lasttime + 1000.0/FPS - SDL_GetTicks();
    if(t > 0)
        SDL_Delay(t);

    *lasttime = SDL_GetTicks();
}

void calc_fps(FPSCounter *fps) {
    if(!fps->stagebg_fps)
        fps->stagebg_fps = FPS;

    if(SDL_GetTicks() > fps->fpstime+1000) {
        fps->show_fps = fps->fps;
        fps->fps = 0;
        fps->fpstime = SDL_GetTicks();
    } else {
        fps->fps++;
    }

    fps->stagebg_fps = approach(fps->stagebg_fps, fps->show_fps, 0.1);
}

void set_ortho(void) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, SCREEN_W, SCREEN_H, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_DEPTH_TEST);
}

void colorfill(float r, float g, float b, float a) {
    if(a <= 0) return;

    glColor4f(r,g,b,a);

    glPushMatrix();
    glScalef(SCREEN_W,SCREEN_H,1);
    glTranslatef(0.5,0.5,0);

    draw_quad();
    glPopMatrix();

    glColor4f(1,1,1,1);
}

void fade_out(float f) {
    colorfill(0, 0, 0, f);
}

//
// i/o uitls
//

char* read_all(const char *filename, int *outsize) {
    char *text;
    size_t size;

    FILE *file = fopen(filename, "r");
    if(file == NULL)
        errx(-1, "Error opening '%s'", filename);

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);

    text = malloc(size+1);
    fread(text, size, 1, file);
    text[size] = 0;

    fclose(file);

    if(outsize) {
        *outsize = size;
    }

    return text;
}

//
// misc utils
//

int getenvint(const char *v) {
    char *e = getenv(v);

    if(e) {
        return atoi(e);
    }

    return 0;
}
