
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

char* vstrfmt(const char *fmt, va_list args) {
    size_t written = 0;
    size_t fmtlen = strlen(fmt);
    size_t asize = 1;
    char *out = NULL;

    while(asize * 2 <= fmtlen)
        asize *= 2;

    do {
        asize *= 2;
        out = realloc(out, asize);
        va_list nargs;
        va_copy(nargs, args);
        written = vsnprintf(out, asize, fmt, nargs);
        va_end(nargs);

    } while(written >= asize);

    return realloc(out, strlen(out) + 1);
}

char* strfmt(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *str = vstrfmt(fmt, args);
    va_end(args);
    return str;
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

int sign(double x) {
    return (x > 0) - (x < 0);
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

    SDL_RWops *file = SDL_RWFromFile(filename, "r");
    if(!file)
        log_fatal("SDL_RWFromFile() failed: %s", SDL_GetError());

    size = SDL_RWsize(file);

    text = malloc(size+1);
    SDL_RWread(file, text, size, 1);
    text[size] = 0;

    SDL_RWclose(file);

    if(outsize) {
        *outsize = size;
    }

    return text;
}

bool parse_keyvalue_stream_cb(SDL_RWops *strm, KVCallback callback, void *data) {
    const size_t bufsize = 128;

#define SYNTAXERROR { log_warn("Syntax error on line %i, aborted! [%s:%i]", line, __FILE__, __LINE__); return false; }
#define BUFFERERROR { log_warn("String exceed the limit of %li, aborted! [%s:%i]", (long int)bufsize, __FILE__, __LINE__); return false; }

    int i = 0, found, line = 0;
    char c, buf[bufsize], key[bufsize], val[bufsize];

    while(c = (char)SDL_ReadU8(strm)) {
        if(c == '#' && !i) {
            i = 0;
            while((char)SDL_ReadU8(strm) != '\n');
        } else if(c == ' ') {
            if(!i) SYNTAXERROR

            buf[i] = 0;
            i = 0;
            strcpy(key, buf);

            found = 0;
            while(c = (char)SDL_ReadU8(strm)) {
                if(c == '=') {
                    if(++found > 1) SYNTAXERROR
                } else if(c != ' ') {
                    if(!found || c == '\n') SYNTAXERROR

                    do {
                        if(c == '\n') {
                            if(!i) SYNTAXERROR

                            buf[i] = 0;
                            i = 0;
                            strcpy(val, buf);
                            found = 0;
                            ++line;

                            callback(key, val, data);
                            break;
                        } else if(c != '\r') {
                            buf[i++] = c;
                            if(i == bufsize)
                                BUFFERERROR
                        }
                    } while(c = (char)SDL_ReadU8(strm));

                    break;
                }
            } if(found) SYNTAXERROR
        } else {
            buf[i++] = c;
            if(i == bufsize)
                BUFFERERROR
        }
    }

    return true;

#undef SYNTAXERROR
#undef BUFFERERROR
}

bool parse_keyvalue_file_cb(const char *filename, KVCallback callback, void *data) {
    SDL_RWops *strm = SDL_RWFromFile(filename, "r");

    if(!strm) {
        log_warn("SDL_RWFromFile() failed: %s", SDL_GetError());
        return false;
    }

    bool status = parse_keyvalue_stream_cb(strm, callback, data);
    SDL_RWclose(strm);
    return status;
}

static void kvcallback_hashtable(const char *key, const char *val, Hashtable *ht) {
    hashtable_set_string(ht, key, strdup((void*)val));
}

Hashtable* parse_keyvalue_stream(SDL_RWops *strm, size_t tablesize) {
    Hashtable *ht = hashtable_new_stringkeys(tablesize);

    if(!parse_keyvalue_stream_cb(strm, (KVCallback)kvcallback_hashtable, ht)) {
        free(ht);
        ht = NULL;
    }

    return ht;
}

Hashtable* parse_keyvalue_file(const char *filename, size_t tablesize) {
    Hashtable *ht = hashtable_new_stringkeys(tablesize);

    if(!parse_keyvalue_file_cb(filename, (KVCallback)kvcallback_hashtable, ht)) {
        free(ht);
        ht = NULL;
    }

    return ht;
}

static void png_rwops_write_data(png_structp png_ptr, png_bytep data, png_size_t length) {
    SDL_RWops *out = png_get_io_ptr(png_ptr);
    SDL_RWwrite(out, data, length, 1);
}

static void png_rwops_flush_data(png_structp png_ptr) {
    // no flush operation in SDL_RWops
}

static void png_rwops_read_data(png_structp png_ptr, png_bytep data, png_size_t length) {
    SDL_RWops *out = png_get_io_ptr(png_ptr);
    SDL_RWread(out, data, length, 1);
}

void png_init_rwops_read(png_structp png, SDL_RWops *rwops) {
    png_set_read_fn(png, rwops, png_rwops_read_data);
}

void png_init_rwops_write(png_structp png, SDL_RWops *rwops) {
    png_set_write_fn(png, rwops, png_rwops_write_data, png_rwops_flush_data);
}

char* SDL_RWgets(SDL_RWops *rwops, char *buf, size_t bufsize) {
    char c, *ptr = buf, *end = buf + bufsize - 1;
    assert(end > ptr);

    while((c = SDL_ReadU8(rwops)) && ptr <= end) {
        if((*ptr++ = c) == '\n')
            break;
    }

    if(ptr == buf)
        return NULL;

    *ptr = 0;
    return buf;
}

size_t SDL_RWprintf(SDL_RWops *rwops, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *str = vstrfmt(fmt, args);
    va_end(args);

    size_t ret = SDL_RWwrite(rwops, str, 1, strlen(str));
    free(str);

    return ret;
}

void tsfprintf(FILE *out, const char *restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);
}

//
// misc utils
//

void _ts_assert_fail(const char *cond, const char *func, const char *file, int line, bool use_log) {
    use_log = use_log && log_initialized();

    if(use_log) {
        log_fatal("%s:%i: %s(): assertion `%s` failed", file, line, func, cond);
    } else {
        tsfprintf(stderr, "%s:%i: %s(): assertion `%s` failed", file, line, func, cond);
        abort();
    }
}

int getenvint(const char *v) {
    char *e = getenv(v);

    if(e) {
        return atoi(e);
    }

    return 0;
}

noreturn static void png_error_handler(png_structp png_ptr, png_const_charp error_msg) {
    log_warn("PNG error: %s", error_msg);
    png_longjmp(png_ptr, 1);
}

static void png_warning_handler(png_structp png_ptr, png_const_charp warning_msg) {
    log_warn("PNG warning: %s", warning_msg);
}

void png_setup_error_handlers(png_structp png) {
    png_set_error_fn(png, NULL, png_error_handler, png_warning_handler);
}
