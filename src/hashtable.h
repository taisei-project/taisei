/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_hashtable_h
#define IGUARD_hashtable_h

#include "taisei.h"

/*
 * hash_t
 *
 * Type of the hash values.
 */
typedef uint32_t hash_t;

/*
 * htutil_init
 *
 * One-time setup function.
 */
void htutil_init(void);

/*
 * htutil_hashfunc_uint32
 *
 * Hash function for 32-bit integers
 * NOTE: assuming hash_t stays uint32_t, this function has no collisions.
 */
static inline attr_must_inline hash_t htutil_hashfunc_uint32(uint32_t x) {
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	return (hash_t)x;
}

/*
 * htutil_hashfunc_uint64
 *
 * Hash function for 64-bit integers.
 */
static inline attr_must_inline hash_t htutil_hashfunc_uint64(uint64_t x) {
	x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
	x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
	x = x ^ (x >> 31);
	return (hash_t)x;
}

/*
 * htutil_hashfunc_string
 *
 * Hash function for null-terminated strings.
 */
extern uint32_t (*htutil_hashfunc_string)(uint32_t crc, const char *str);

// Import public declarations for the predefined hashtable types.
#define HT_DECL
#include "hashtable_predefs.inc.h"

// NOTE: For the hashtable API, see hashtable.inc.h
// NOTE: For type-generic wrappers around the API, see hashtable_predefs.inc.h

#endif // IGUARD_hashtable_h
