/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "util.h"

extern const char *const TAISEI_VERSION;
extern const char *const TAISEI_VERSION_FULL;
extern const char *const TAISEI_VERSION_BUILD_TYPE;

extern const uint8_t  TAISEI_VERSION_MAJOR;
extern const uint8_t  TAISEI_VERSION_MINOR;
extern const uint8_t  TAISEI_VERSION_PATCH;
extern const uint16_t TAISEI_VERSION_TWEAK;

typedef struct TaiseiVersion {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint16_t tweak;
} TaiseiVersion;

#define TAISEI_VERSION_SET(v,ma,mi,pa,tw) { \
    (v)->major = (ma); \
    (v)->minor = (mi); \
    (v)->patch = (pa); \
    (v)->tweak = (tw); \
}

#define TAISEI_VERSION_GET_CURRENT(v) TAISEI_VERSION_SET(v, \
    TAISEI_VERSION_MAJOR, \
    TAISEI_VERSION_MINOR, \
    TAISEI_VERSION_PATCH, \
    TAISEI_VERSION_TWEAK  \
)

typedef enum {
    VCMP_MAJOR,
    VCMP_MINOR,
    VCMP_PATCH,
    VCMP_TWEAK,
} TaiseiVersionCmpLevel;

// this is for IO purposes. sizeof(TaiseiVersion) may not match.
#define TAISEI_VERSION_SIZE (sizeof(uint8_t) * 3 + sizeof(uint16_t))

int taisei_version_compare(TaiseiVersion *v1, TaiseiVersion *v2, TaiseiVersionCmpLevel level);
char* taisei_version_tostring(TaiseiVersion *version);
size_t taisei_version_read(SDL_RWops *rwops, TaiseiVersion *version);
size_t taisei_version_write(SDL_RWops *rwops, TaiseiVersion *version);
