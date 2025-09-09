/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "shaderlib.h"
#include "cache.h"
#include "reflect.h"

#include "log.h"
#include "util/sha256.h"
#include "rwops/rwops_crc32.h"
#include "rwops/rwops_autobuf.h"
#include "rwops/rwops_zstd.h"
#include "vfs/public.h"

#define CACHE_VERSION 8
#define CRC_INIT 0

#define MAX_CONTENT_SIZE          (1024 * 1024)
#define MAX_GLSL_MACROS           255
#define MAX_GLSL_MACRO_NAME_LEN   255
#define MAX_GLSL_MACRO_VALUE_LEN  255
#define MAX_ENTRYPOINT_NAME_LEN   255

static uint8_t *shader_cache_construct_entry(const ShaderSource *src, const ShaderMacro *macros, size_t *out_size) {
	uint8_t *buf, *result = NULL;
	uint32_t crc = CRC_INIT;

	SDL_IOStream *abuf = SDL_RWAutoBuffer((void**)&buf, BUFSIZ);
	SDL_IOStream *crcbuf = SDL_RWWrapCRC32(abuf, &crc, false);
	SDL_IOStream *dest = crcbuf;

	if(UNLIKELY(!crcbuf)) {
		log_sdl_error(LOG_ERROR, "SDL_RWWrapCRC32");
		return NULL;
	}

	// TODO: check for IO errors

	SDL_WriteU8(dest, CACHE_VERSION);
	SDL_WriteU8(dest, src->stage);
	SDL_WriteU8(dest, src->lang.lang);

	size_t entrypoint_name_len = strlen(src->entrypoint);

	if(entrypoint_name_len > MAX_ENTRYPOINT_NAME_LEN) {
		log_error("Entry point name is too long (%zu): %s", entrypoint_name_len, src->entrypoint);
		goto fail;
	}

	SDL_WriteU8(dest, entrypoint_name_len);
	SDL_WriteIO(dest, src->entrypoint, entrypoint_name_len);

	if(macros) {
		uint num_macros = 0;

		for(const ShaderMacro *m = macros; m->name; ++m) {
			++num_macros;
		}

		if(num_macros > MAX_GLSL_MACROS) {
			log_error("Too many macros (%i)", num_macros);
			goto fail;
		}

		SDL_WriteU8(dest, num_macros);

		for(const ShaderMacro *m = macros; m->name; ++m) {
			size_t name_len = strlen(m->name);
			size_t value_len = strlen(m->value);

			if(name_len > MAX_GLSL_MACRO_NAME_LEN) {
				log_error("Macro name is too long (%zu): %s", name_len, m->name);
				goto fail;
			}

			if(value_len > MAX_GLSL_MACRO_VALUE_LEN) {
				log_error("Macro value is too long (%zu): %s", name_len, m->name);
				goto fail;
			}

			SDL_WriteU8(dest, name_len);
			SDL_WriteIO(dest, m->name, name_len);

			SDL_WriteU8(dest, value_len);
			SDL_WriteIO(dest, m->value, value_len);
		}
	} else {
		SDL_WriteU8(dest, 0);
	}

	switch(src->lang.lang) {
		case SHLANG_GLSL: {
			SDL_WriteU16LE(dest, src->lang.glsl.version.version);
			SDL_WriteU8(dest, src->lang.glsl.version.profile);
			break;
		}

		case SHLANG_SPIRV: {
			SDL_WriteU8(dest, src->lang.spirv.target);
			break;
		}

		case SHLANG_HLSL: {
			SDL_WriteU8(dest, src->lang.hlsl.shader_model);
			break;
		}

		case SHLANG_DXBC: {
			SDL_WriteU8(dest, src->lang.dxbc.shader_model);
			break;
		}

		case SHLANG_MSL: {
			break;
		}

		default: {
			log_error("Unhandled shading language %s", shader_lang_name(src->lang.lang));
			goto fail;
		}
	}

	if(src->content_size > MAX_CONTENT_SIZE) {
		log_error("Content is too large (%zu)", src->content_size);
		goto fail;
	}

	if(!shader_reflection_serialize(src->reflection, dest)) {
		goto fail;
	}

	SDL_WriteU32LE(dest, src->content_size);
	SDL_WriteIO(dest, src->content, src->content_size);

	dest = abuf;
	SDL_CloseIO(crcbuf);
	crcbuf = NULL;

	SDL_WriteU32LE(dest, crc);

	*out_size = SDL_TellIO(dest);
	result = memdup(buf, *out_size);

fail:
	if(crcbuf != NULL) {
		SDL_CloseIO(crcbuf);
	}

	SDL_CloseIO(abuf);
	return result;
}

#define READ(_file, _func, _type) ({ \
	_type _tmp = 0; \
	if(UNLIKELY(!(_func)((_file), &_tmp))) { \
		log_error("Read error: %s", SDL_GetError()); \
	} \
	_tmp; \
})

#define READU8(_file)    READ(_file, SDL_ReadU8,    uint8_t)
#define READU16LE(_file) READ(_file, SDL_ReadU16LE, uint16_t)
#define READU32LE(_file) READ(_file, SDL_ReadU32LE, uint32_t)

static bool shader_cache_load_entry(SDL_IOStream *stream, ShaderSource *out_src, MemArena *arena) {
	uint32_t crc = CRC_INIT;
	SDL_IOStream *s = SDL_RWWrapCRC32(stream, &crc, false);

	auto arena_snap = marena_snapshot(arena);

	*out_src = (typeof(*out_src)) {};

	// TODO check for IO errors

	if(READU8(s) != CACHE_VERSION) {
		log_error("Version mismatch");
		goto fail;
	}

	out_src->stage = READU8(s);
	out_src->lang = (typeof(out_src->lang)) {
		.lang = READU8(s)
	};

	uint32_t entrypoint_len = READU8(s);
	char *entrypoint = marena_alloc(arena, entrypoint_len + 1);
	entrypoint[entrypoint_len] = 0;

	if(SDL_ReadIO(s, entrypoint, entrypoint_len) != entrypoint_len) {
		log_sdl_error(LOG_ERROR, "SDL_ReadIO");
		goto fail;
	}

	out_src->entrypoint = entrypoint;

	if(READU8(s) > 0) {
		log_error("Cache entry contains macros, this is not implemented");
		goto fail;
	}

	switch(out_src->lang.lang) {
		case SHLANG_GLSL: {
			out_src->lang.glsl.version.version = READU16LE(s);
			out_src->lang.glsl.version.profile = READU8(s);
			break;
		}

		case SHLANG_SPIRV: {
			out_src->lang.spirv.target = READU8(s);
			break;
		}

		case SHLANG_HLSL: {
			out_src->lang.hlsl.shader_model = READU8(s);
			break;
		}

		case SHLANG_DXBC: {
			out_src->lang.dxbc.shader_model = READU8(s);
			break;
		}

		case SHLANG_MSL: {
			break;
		}

		default: {
			log_error("Unhandled shading language %s", shader_lang_name(out_src->lang.lang));
			goto fail;
		}
	}

	if(!shader_reflection_deserialize(&out_src->reflection, arena, s)) {
		goto fail;
	}

	uint32_t content_size = READU32LE(s);
	char *content = marena_alloc(arena, content_size);

	if(SDL_ReadIO(s, content, content_size) != content_size) {
		log_sdl_error(LOG_ERROR, "SDL_ReadIO");
		goto fail;
	}

	out_src->content_size = content_size;
	out_src->content = content;

	uint32_t file_crc = READU32LE(stream);

	if(crc != file_crc) {
		log_error("CRC mismatch (%08x != %08x), cache entry is corrupted", crc, file_crc);
		goto fail;
	}

	SDL_CloseIO(s);
	return true;

fail:
	marena_rollback(arena, &arena_snap);
	SDL_CloseIO(s);
	return false;
}

bool shader_cache_get(const char *hash, const char *key, ShaderSource *entry, MemArena *arena) {
	char path[256];
	snprintf(path, sizeof(path), "cache/shaders/%s/%s", hash, key);

	SDL_IOStream *stream = vfs_open(path, VFS_MODE_READ);

	if(stream == NULL) {
		return false;
	}

	stream = NOT_NULL(SDL_RWWrapZstdReader(stream, true));
	bool result = shader_cache_load_entry(stream, entry, arena);
	SDL_CloseIO(stream);

	log_debug("%s %s/%s from cache", result ? "Retrieved " : "Failed to retrieve",  hash, key);
	return result;
}

static bool shader_cache_set_raw(const char *hash, const char *key, uint8_t *entry, size_t entry_size) {
	char path[256];

	vfs_mkdir("cache/shaders");
	snprintf(path, sizeof(path), "cache/shaders/%s", hash);
	vfs_mkdir(path);
	snprintf(path, sizeof(path), "cache/shaders/%s/%s", hash, key);

	SDL_IOStream *out = vfs_open(path, VFS_MODE_WRITE);

	if(out == NULL) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	out = NOT_NULL(SDL_RWWrapZstdWriter(out, RW_ZSTD_LEVEL_DEFAULT, true));
	SDL_WriteIO(out, entry, entry_size);
	SDL_CloseIO(out);

	return true;
}

bool shader_cache_set(const char *hash, const char *key, const ShaderSource *src) {
	size_t entry_size;
	uint8_t *entry = shader_cache_construct_entry(src, NULL, &entry_size);

	if(entry != NULL) {
		shader_cache_set_raw(hash, key, entry, entry_size);
		mem_free(entry);
		log_debug("Stored %s/%s in cache", hash, key);
		return true;
	}

	return false;
}

bool shader_cache_hash(const ShaderSource *src, const ShaderMacro *macros, size_t buf_size, char out_buf[buf_size]) {
	assert(buf_size >= SHADER_CACHE_HASH_BUFSIZE);

	size_t entry_size;
	uint8_t *entry = shader_cache_construct_entry(src, macros, &entry_size);

	if(entry == NULL) {
		return false;
	}

	const size_t sha_size = SHA256_BLOCK_SIZE*2;

	sha256_hexdigest(entry, entry_size, out_buf, buf_size);
	snprintf(out_buf + sha_size, buf_size - sha_size, "-%zx", entry_size);

	// shader_cache_set_raw(out_buf, "orig", entry, entry_size);

	mem_free(entry);
	return true;
}
