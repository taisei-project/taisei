/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

typedef uint_fast16_t bitarray_word_t;

#define BITARRAY_WORD_BITS (sizeof(bitarray_word_t) * 8)

#define BITARRAY_NUM_WORDS_FROM_BITS(N) \
    ((N) + (BITARRAY_WORD_BITS - 1)) / BITARRAY_WORD_BITS + ((N) == 1)

#define BIT_ARRAY(N) struct { \
    bitarray_word_t words[BITARRAY_NUM_WORDS_FROM_BITS(N)]; \
}

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*(arr)))

INLINE bool _bitarray_get(uint nwords, bitarray_word_t words[nwords], uint bit) {
    return (bool)(1 & (words[bit / BITARRAY_WORD_BITS] >> (bit & (BITARRAY_WORD_BITS - 1))));
}

INLINE bool _bitarray_set(uint nwords, bitarray_word_t words[nwords], uint bit, bool val) {
    bitarray_word_t *w = words + (bit / BITARRAY_WORD_BITS);
    bitarray_word_t mask = ((bitarray_word_t)1 << (bit & (BITARRAY_WORD_BITS - 1)));
    *w = (*w & ~mask) | (mask * val);
    return val;
}

#define bitarray_get(bitarray, bit) \
    _bitarray_get(ARRAY_SIZE((bitarray)->words), (bitarray)->words, (bit))

#define bitarray_set(bitarray, bit, val) \
    _bitarray_set(ARRAY_SIZE((bitarray)->words), (bitarray)->words, (bit), (val))
