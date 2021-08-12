/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util.h"

#ifdef TAISEI_BUILDCONF_DEBUG
	#define TAISEI_VERSION_BUILD_TYPE_0 "debug"
#else
	#define TAISEI_VERSION_BUILD_TYPE_0 "release"
#endif

#ifdef TAISEI_BUILDCONF_DEVELOPER
	#define TAISEI_VERSION_BUILD_TYPE_1 " devbuild"
#else
	#define TAISEI_VERSION_BUILD_TYPE_1 ""
#endif

#ifdef NDEBUG
	#define TAISEI_VERSION_BUILD_TYPE_2 ""
#else
	#define TAISEI_VERSION_BUILD_TYPE_2 " assert"
#endif

#define TAISEI_VERSION_BUILD_TYPE \
	TAISEI_VERSION_BUILD_TYPE_0 \
	TAISEI_VERSION_BUILD_TYPE_1 \
	TAISEI_VERSION_BUILD_TYPE_2 \

extern const char *const TAISEI_VERSION;
extern const char *const TAISEI_VERSION_FULL;

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
	// Start filling this enum from nonzero value.
	// This would help avoid a warning on some compilers like
	// those with EDG frontend. Actual values of VCMP_...
	// don't matter, the thing which does matter is its order.
	VCMP_MAJOR = 1,
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
