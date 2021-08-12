/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

enum {
	SHA256_BLOCK_SIZE = 32,
	SHA256_HEXDIGEST_SIZE = SHA256_BLOCK_SIZE * 2 + 1,
};

typedef struct SHA256State SHA256State;

SHA256State *sha256_new(void) attr_returns_allocated;
void sha256_update(SHA256State *state, const uint8_t *data, size_t len) attr_nonnull(1, 2);
void sha256_final(SHA256State *state, uint8_t hash[SHA256_BLOCK_SIZE], size_t hashlen) attr_nonnull(1, 2);
void sha256_free(SHA256State *state);

void sha256_digest(const uint8_t *data, size_t len, uint8_t hash[SHA256_BLOCK_SIZE], size_t hashlen);
void sha256_hexdigest(const uint8_t *data, size_t len, char hash[SHA256_HEXDIGEST_SIZE], size_t hashlen);
