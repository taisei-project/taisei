/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "serialize.h"

#include "rwops/rwops_crc32.h"
#include "util.h"

static const uint8_t pxs_magic[] = { 0xe2, 0x99, 0xa5, 0x52, 0x65, 0x69, 0x6d, 0x75 };
#define PXS_CRC32_INIT 42069

bool pixmap_serialize(SDL_RWops *stream, const Pixmap *pixmap) {
	SDL_RWops *cstream = NULL;

	#define CHECK_WRITE(expr, expected_size) \
		do { \
			if(UNLIKELY((expr) != 1)) { \
				log_error("Write of %zu bytes failed: %s", (size_t)(expected_size), SDL_GetError()); \
				goto fail; \
			} \
		} while(0)

	#define W_BYTES(stream, data, size) CHECK_WRITE(SDL_RWwrite(stream, data, size, 1), size)
	#define W_U32(stream, val) CHECK_WRITE(SDL_WriteLE32(stream, val), sizeof(uint32_t))
	#define W_U16(stream, val) CHECK_WRITE(SDL_WriteLE16(stream, val), sizeof(uint16_t))
	#define W_U8(stream, val)  CHECK_WRITE(SDL_WriteU8(stream, val), sizeof(uint8_t))

	W_BYTES(stream, pxs_magic, sizeof(pxs_magic));
	W_U16(stream, PIXMAP_SERIALIZED_VERSION);

	uint32_t crc32 = PXS_CRC32_INIT;
	cstream = NOT_NULL(SDL_RWWrapCRC32(stream, &crc32, false));

	W_U32(cstream, pixmap->width);
	W_U32(cstream, pixmap->height);
	W_U32(cstream, pixmap->data_size);
	W_U16(cstream, pixmap->format);
	W_U8(cstream, pixmap->origin);
	W_BYTES(cstream, pixmap->data.untyped, pixmap->data_size);

	SDL_RWclose(cstream);
	cstream = NULL;

	W_U32(stream, crc32);

	return true;

fail:
	if(cstream) {
		SDL_RWclose(cstream);
	}

	return false;
}

bool pixmap_deserialize(SDL_RWops *stream, Pixmap *pixmap) {
	assume(pixmap->data.untyped == NULL);

	SDL_RWops *cstream = NULL;

	uint8_t magic[sizeof(pxs_magic)] = { 0 };
	SDL_RWread(stream, magic, 1, sizeof(magic));

	if(memcmp(magic, pxs_magic, sizeof(magic))) {
		log_error("Bad magic header");
		return false;
	}

	uint16_t version = SDL_ReadLE16(stream);
	if(version != PIXMAP_SERIALIZED_VERSION) {
		log_error("Bad version %u; expected %u", version, PIXMAP_SERIALIZED_VERSION);
		return false;
	}

	uint32_t crc32 = PXS_CRC32_INIT;
	cstream = NOT_NULL(SDL_RWWrapCRC32(stream, &crc32, false));

	// TODO validate and verify consistency of data_size/width/height/format

	pixmap->width = SDL_ReadLE32(cstream);
	pixmap->height = SDL_ReadLE32(cstream);
	pixmap->data_size = SDL_ReadLE32(cstream);
	pixmap->format = SDL_ReadLE16(cstream);
	pixmap->origin = SDL_ReadU8(cstream);

	if(pixmap->data_size > PIXMAP_BUFFER_MAX_SIZE) {
		log_error("Data size is too large");
		goto fail;
	}

	if(
		pixmap->origin != PIXMAP_ORIGIN_BOTTOMLEFT &&
		pixmap->origin != PIXMAP_ORIGIN_TOPLEFT
	) {
		log_error("Invalid origin 0x%x", pixmap->origin);
		goto fail;
	}

	pixmap->data.untyped = NOT_NULL(malloc(pixmap->data_size));
	size_t read = SDL_RWread(cstream, pixmap->data.untyped, 1, pixmap->data_size);

	if(read != pixmap->data_size) {
		log_error("Read %zu bytes, expected %u", read, pixmap->data_size);
		goto fail;
	}

	SDL_RWclose(cstream);
	cstream = NULL;

	uint32_t crc32_in = SDL_ReadLE32(stream);

	if(crc32_in != crc32) {
		log_error("CRC32 mismatch: 0x%08x != 0x%08x", crc32_in, crc32);
		goto fail;
	}

	return true;

fail:
	if(pixmap->data.untyped) {
		free(pixmap->data.untyped);
		pixmap->data.untyped = NULL;
	}

	if(cstream) {
		SDL_RWclose(cstream);
	}

	return false;
}
