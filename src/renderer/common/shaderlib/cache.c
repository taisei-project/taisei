/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "shaderlib.h"

#include "util.h"
#include "util/sha256.h"
#include "rwops/rwops_crc32.h"
#include "rwops/rwops_autobuf.h"
#include "rwops/rwops_zstd.h"

#define CACHE_VERSION 5
#define CRC_INIT 0

#define MAX_CONTENT_SIZE          (1024 * 1024)
#define MAX_GLSL_ATTRIBS          255
#define MAX_GLSL_MACROS           255
#define MAX_GLSL_MACRO_NAME_LEN   255
#define MAX_GLSL_MACRO_VALUE_LEN  255

static uint8_t *shader_cache_construct_entry(const ShaderSource *src, const ShaderMacro *macros, size_t *out_size) {
	uint8_t *buf, *result = NULL;
	uint32_t crc = CRC_INIT;

	SDL_RWops *abuf = SDL_RWAutoBuffer((void**)&buf, BUFSIZ);
	SDL_RWops *crcbuf = SDL_RWWrapCRC32(abuf, &crc, false);
	SDL_RWops *dest = crcbuf;

	if(UNLIKELY(!crcbuf)) {
		log_sdl_error(LOG_ERROR, "SDL_RWWrapCRC32");
		return NULL;
	}

	SDL_WriteU8(dest, CACHE_VERSION);
	SDL_WriteU8(dest, src->stage);
	SDL_WriteU8(dest, src->lang.lang);

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
			SDL_RWwrite(dest, m->name, name_len, 1);

			SDL_WriteU8(dest, value_len);
			SDL_RWwrite(dest, m->value, value_len, 1);
		}
	} else {
		SDL_WriteU8(dest, 0);
	}

	switch(src->lang.lang) {
		case SHLANG_GLSL: {
			SDL_WriteLE16(dest, src->lang.glsl.version.version);
			SDL_WriteU8(dest, src->lang.glsl.version.profile);

			if(src->meta.glsl.num_attributes > MAX_GLSL_ATTRIBS) {
				log_error("Too many attributes (%i)", src->meta.glsl.num_attributes);
				goto fail;
			}

			SDL_WriteU8(dest, src->meta.glsl.num_attributes);

			for(uint i = 0; i < src->meta.glsl.num_attributes; ++i) {
				SDL_WriteLE32(dest, src->meta.glsl.attributes[i].location);

				size_t len = strlen(src->meta.glsl.attributes[i].name);

				if(len > 255) {
					log_error("Attribute name is too long (%zu): %s", len, src->meta.glsl.attributes[i].name);
					goto fail;
				}

				SDL_WriteU8(dest, len);
				SDL_RWwrite(dest, src->meta.glsl.attributes[i].name, len, 1);
			}
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

	if(src->content_size > MAX_CONTENT_SIZE) {
		log_error("Content is too large (%zu)", src->content_size);
		goto fail;
	}

	SDL_WriteLE32(dest, src->content_size);
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

	memset(out_src, 0, sizeof(*out_src));
	out_src->content = NULL;

	if(SDL_ReadU8(s) != CACHE_VERSION) {
		log_error("Version mismatch");
		goto fail;
	}

	out_src->stage = SDL_ReadU8(s);

	memset(&out_src->lang, 0, sizeof(out_src->lang));
	out_src->lang.lang = SDL_ReadU8(s);

	if(SDL_ReadU8(s) > 0) {
		log_error("Cache entry contains macros, this is not implemented");
		goto fail;
	}

	switch(out_src->lang.lang) {
		case SHLANG_GLSL: {
			out_src->lang.glsl.version.version = SDL_ReadLE16(s);
			out_src->lang.glsl.version.profile = SDL_ReadU8(s);

			uint nattrs = SDL_ReadU8(s);

			if(nattrs > 0) {
				out_src->meta.glsl.num_attributes = nattrs;
				out_src->meta.glsl.attributes = ALLOC_ARRAY(nattrs, typeof(*out_src->meta.glsl.attributes));

				for(uint i = 0; i < nattrs; ++i) {
					GLSLAttribute *attr = out_src->meta.glsl.attributes + i;
					attr->location = SDL_ReadLE32(s);
					uint len = SDL_ReadU8(s);
					attr->name = mem_alloc(len + 1);
					SDL_RWread(s, attr->name, len, 1);
				}
			}
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

	out_src->content_size = SDL_ReadLE32(s);
	out_src->content = mem_alloc(out_src->content_size);

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
	shader_free_source(out_src);
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

	stream = NOT_NULL(SDL_RWWrapZstdReader(stream, true));
	bool result = shader_cache_load_entry(stream, entry);
	SDL_RWclose(stream);

	log_debug("Retrieved %s/%s from cache", hash, key);
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

	out = NOT_NULL(SDL_RWWrapZstdWriter(out, RW_ZSTD_LEVEL_DEFAULT, true));
	SDL_RWwrite(out, entry, entry_size, 1);
	SDL_RWclose(out);

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
