/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "shaderlib.h"

#include "util.h"
#include "util/sha256.h"
#include "rwops/all.h"

#define CACHE_VERSION 1
#define CRC_INIT 0

static uint8_t* shader_cache_construct_entry(const ShaderSource *src, size_t *out_size) {
	uint8_t *buf, *result = NULL;
	uint32_t crc = CRC_INIT;

	SDL_RWops *abuf = SDL_RWAutoBuffer((void**)&buf, BUFSIZ);
	SDL_RWops *crcbuf = SDL_RWWrapCRC32(abuf, &crc, false);
	SDL_RWops *dest = crcbuf;

	SDL_WriteU8(dest, CACHE_VERSION);
	SDL_WriteU8(dest, src->stage);
	SDL_WriteU8(dest, src->lang.lang);

	switch(src->lang.lang) {
		case SHLANG_GLSL: {
			SDL_WriteLE16(dest, src->lang.glsl.version.version);
			SDL_WriteU8(dest, src->lang.glsl.version.profile);
			break;
		}

		case SHLANG_SPIRV: {
			SDL_WriteU8(dest, src->lang.spirv.target);
			break;
		}

		default: {
			log_error("Unhandled shading language id=%u", src->lang.lang);
			goto fail;
		}
	}

	SDL_WriteLE64(dest, src->content_size);
	SDL_RWwrite(dest, src->content, src->content_size, 1);

	dest = abuf;
	SDL_RWclose(crcbuf);
	crcbuf = NULL;

	SDL_WriteLE32(dest, crc);

	*out_size = SDL_RWtell(dest);
	result = memdup(buf, *out_size);

fail:
	if(crcbuf != NULL) {
		SDL_RWclose(crcbuf);
	}

	SDL_RWclose(abuf);
	return result;
}

static bool shader_cache_load_entry(SDL_RWops *stream, ShaderSource *out_src) {
	uint32_t crc = CRC_INIT;
	SDL_RWops *s = SDL_RWWrapCRC32(stream, &crc, false);

	out_src->content = NULL;

	if(SDL_ReadU8(s) != CACHE_VERSION) {
		log_error("Version mismatch");
		goto fail;
	}

	out_src->stage = SDL_ReadU8(s);

	memset(&out_src->lang, 0, sizeof(out_src->lang));
	out_src->lang.lang = SDL_ReadU8(s);

	switch(out_src->lang.lang) {
		case SHLANG_GLSL: {
			out_src->lang.glsl.version.version = SDL_ReadLE16(s);
			out_src->lang.glsl.version.profile = SDL_ReadU8(s);
			break;
		}

		case SHLANG_SPIRV: {
			out_src->lang.spirv.target = SDL_ReadU8(s);
			break;
		}

		default: {
			log_error("Unhandled shading language id=%u", out_src->lang.lang);
			goto fail;
		}
	}

	out_src->content_size = SDL_ReadLE64(s);
	out_src->content = calloc(1, out_src->content_size);

	if(SDL_RWread(s, out_src->content, out_src->content_size, 1) != 1) {
		log_error("Read error");
		goto fail;
	}

	uint32_t file_crc = SDL_ReadLE32(stream);

	if(crc != file_crc) {
		log_error("CRC mismatch (%08x != %08x), cache entry is corrupted", crc, file_crc);
		goto fail;
	}

	SDL_RWclose(s);
	return true;

fail:
	free(out_src->content);
	out_src->content = NULL;
	SDL_RWclose(s);
	return false;
}

bool shader_cache_get(const char *hash, const char *key, ShaderSource *entry) {
	char path[256];
	snprintf(path, sizeof(path), "cache/shaders/%s/%s", hash, key);

	SDL_RWops *stream = vfs_open(path, VFS_MODE_READ);

	if(stream == NULL) {
		return false;
	}

	stream = SDL_RWWrapZReader(stream, BUFSIZ, true);
	bool result = shader_cache_load_entry(stream, entry);
	SDL_RWclose(stream);

	return result;
}

static bool shader_cache_set_raw(const char *hash, const char *key, uint8_t *entry, size_t entry_size) {
	char path[256];

	vfs_mkdir("cache/shaders");
	snprintf(path, sizeof(path), "cache/shaders/%s", hash);
	vfs_mkdir(path);
	snprintf(path, sizeof(path), "cache/shaders/%s/%s", hash, key);

	SDL_RWops *out = vfs_open(path, VFS_MODE_WRITE);

	if(out == NULL) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	out = SDL_RWWrapZWriter(out, BUFSIZ, true);
	assert(out != NULL);

	SDL_RWwrite(out, entry, entry_size, 1);
	SDL_RWclose(out);

	return true;
}

bool shader_cache_set(const char *hash, const char *key, const ShaderSource *src) {
	size_t entry_size;
	uint8_t *entry = shader_cache_construct_entry(src, &entry_size);

	if(entry != NULL) {
		shader_cache_set_raw(hash, key, entry, entry_size);
		free(entry);
		return true;
	}

	return false;
}

bool shader_cache_hash(const ShaderSource *src, char *out_buf, size_t bufsize) {
	assert(bufsize >= SHADER_CACHE_HASH_BUFSIZE);

	size_t entry_size;
	uint8_t *entry = shader_cache_construct_entry(src, &entry_size);

	if(entry == NULL) {
		return false;
	}

	const size_t sha_size = SHA256_BLOCK_SIZE*2;

	sha256_hexdigest(entry, entry_size, out_buf, bufsize);
	snprintf(out_buf + sha_size, bufsize - sha_size, "-%08zu", entry_size);

	// shader_cache_set_raw(out_buf, "orig", entry, entry_size);

	free(entry);
	return true;
}
