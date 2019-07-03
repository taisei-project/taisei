/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "sha256.h"
#include "util.h"

#include <openssl/sha.h>

struct SHA256State {
	SHA256_CTX context;
};

SHA256State* sha256_new(void) {
	SHA256State *st = calloc(1, sizeof(*st));
	SHA256_Init(&st->context);
	return st;
}

void sha256_free(SHA256State *ctx) {
	free(ctx);
}

void sha256_update(SHA256State *ctx, const uint8_t *data, size_t len) {
    SHA256_Update(&ctx->context, data, len);
}

void sha256_final(SHA256State *ctx, uint8_t hash[SHA256_BLOCK_SIZE], size_t hashlen) {
	assert(hashlen >= SHA256_BLOCK_SIZE);
	SHA256_Final(hash, &ctx->context);
}

void sha256_digest(const uint8_t *data, size_t len, uint8_t hash[SHA256_BLOCK_SIZE], size_t hashlen) {
	assert(hashlen >= SHA256_BLOCK_SIZE);

	SHA256_CTX ctx;
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, data, len);
	SHA256_Final(hash, &ctx);
}

void sha256_hexdigest(const uint8_t *data, size_t len, char hash[SHA256_BLOCK_SIZE*2+1], size_t hashlen) {
	assert(hashlen >= SHA256_BLOCK_SIZE * 2 + 1);

	uint8_t digest[SHA256_BLOCK_SIZE];
	sha256_digest(data, len, digest, sizeof(digest));
	hexdigest(digest, sizeof(digest), hash, hashlen);
}
