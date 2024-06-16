/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

/*
 * hash_t
 *
 * Type of the hash values.
 */
typedef uint32_t hash_t;

#define HT_HASH_LIVE_BIT ((hash_t)(1) << (sizeof(hash_t) * CHAR_BIT - 1))

typedef hash_t ht_size_t;

// for hashsets
struct ht_empty { };
#define HT_EMPTY ((struct ht_empty) {})

/*
 * htutil_hashfunc_uint32
 *
 * Hash function for 32-bit integers
 * NOTE: assuming hash_t stays uint32_t, this function has no collisions.
 */
INLINE hash_t htutil_hashfunc_uint32(uint32_t x) {
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
INLINE hash_t htutil_hashfunc_uint64(uint64_t x) {
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
INLINE uint32_t htutil_hashfunc_string(const char *str) {
	// FNV1a hash
	//
	// Not looking for '\0' in the for loop because apparently compilers optimize this better.
	// Both GCC and clang can fold this entire function into a constant with a constant str.
	// GCC fails that if strlen is not used.

	uint32_t hash = 0x811c9dc5;
	uint32_t prime = 0x1000193;
	size_t len = strlen(str);

	for(int i = 0; i < len; ++i) {
		uint8_t value = str[i];
		hash = hash ^ value;
		hash *= prime;
	}

	return hash;
}

// Import public declarations for the predefined hashtable types.
#define HT_DECL
#include "hashtable_predefs.inc.h"

// NOTE: For the hashtable API, see hashtable.inc.h
// NOTE: For type-generic wrappers around the API, see hashtable_predefs.inc.h
