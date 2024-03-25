/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "sha256.h"
#include "util.h"

#include <openssl/evp.h>

SHA256State *sha256_new(void) {
	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	EVP_DigestInit(ctx, EVP_sha256());
	return (SHA256State*)ctx;
}

void sha256_free(SHA256State *st) {
	EVP_MD_CTX_free((EVP_MD_CTX*)st);
}

void sha256_update(SHA256State *st, const uint8_t *data, size_t len) {
	EVP_DigestUpdate((EVP_MD_CTX*)st, data, len);
}

void sha256_final(SHA256State *st, uint8_t hash[SHA256_BLOCK_SIZE], size_t hashlen) {
	assert(hashlen >= SHA256_BLOCK_SIZE);
	uint osize = 0;
	EVP_DigestFinal_ex((EVP_MD_CTX*)st, hash, &osize);
	assert(osize == SHA256_BLOCK_SIZE);
}

void sha256_digest(const uint8_t *data, size_t len, uint8_t hash[SHA256_BLOCK_SIZE], size_t hashlen) {
	assert(hashlen >= SHA256_BLOCK_SIZE);

	SHA256State *st = sha256_new();
	sha256_update(st, data, len);
	sha256_final(st, hash, hashlen);
	sha256_free(st);
}

void sha256_hexdigest(const uint8_t *data, size_t len, char hash[SHA256_HEXDIGEST_SIZE], size_t hashlen) {
	assert(hashlen >= SHA256_HEXDIGEST_SIZE);

	uint8_t digest[SHA256_BLOCK_SIZE];
	sha256_digest(data, len, digest, sizeof(digest));
	hexdigest(digest, sizeof(digest), hash, hashlen);
}
