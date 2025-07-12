/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "fileformats.h"

#include "log.h"
#include "rwops/rwops_crc32.h"
#include "util.h"

enum {
	PXI_VERSION = 1,
	PXI_CRC32_INIT = 42069,
};

static const uint8_t pxi_magic[] = { 0xe2, 0x99, 0xa5, 0x52, 0x65, 0x69, 0x6d, 0x75 };

static bool px_internal_probe(SDL_IOStream *stream) {
	uint8_t magic[sizeof(pxi_magic)] = { 0 };
	SDL_ReadIO(stream, magic, sizeof(magic));
	return !memcmp(magic, pxi_magic, sizeof(magic));
}

static bool px_internal_save(SDL_IOStream *stream, const Pixmap *pixmap,
			     const PixmapSaveOptions *opts) {
	SDL_IOStream *cstream = NULL;

	#define CHECK_WRITE(expr, expected_size) \
		do { \
			if(UNLIKELY((expr) != 1)) { \
				log_error("Write of %zu bytes failed: %s", (size_t)(expected_size), SDL_GetError()); \
				goto fail; \
			} \
		} while(0)

	#define W_BYTES(stream, data, size) CHECK_WRITE(SDL_WriteIO(stream, data, size) == size, size)
	#define W_U32(stream, val) CHECK_WRITE(SDL_WriteU32LE(stream, val), sizeof(uint32_t))
	#define W_U16(stream, val) CHECK_WRITE(SDL_WriteU16LE(stream, val), sizeof(uint16_t))
	#define W_U8(stream, val)  CHECK_WRITE(SDL_WriteU8(stream, val), sizeof(uint8_t))

	W_BYTES(stream, pxi_magic, sizeof(pxi_magic));
	W_U16(stream, PXI_VERSION);

	uint32_t crc32 = PXI_CRC32_INIT;
	cstream = NOT_NULL(SDL_RWWrapCRC32(stream, &crc32, false));

	W_U32(cstream, pixmap->width);
	W_U32(cstream, pixmap->height);
	W_U32(cstream, pixmap->data_size);
	W_U16(cstream, pixmap->format);
	W_U8(cstream, pixmap->origin);
	W_BYTES(cstream, pixmap->data.untyped, pixmap->data_size);

	SDL_CloseIO(cstream);
	cstream = NULL;

	W_U32(stream, crc32);

	return true;

fail:
	if(cstream) {
		SDL_CloseIO(cstream);
	}

	return false;
}

static bool px_internal_load(SDL_IOStream *stream, Pixmap *pixmap,
			     PixmapFormat preferred_format) {
	assume(pixmap->data.untyped == NULL);

	SDL_IOStream *cstream = NULL;

	// TODO check for read errors

	uint8_t magic[sizeof(pxi_magic)] = { 0 };
	SDL_ReadIO(stream, magic, sizeof(magic));

	if(memcmp(magic, pxi_magic, sizeof(magic))) {
		log_error("Bad magic header");
		return false;
	}

	uint16_t version = 0;
	SDL_ReadU16LE(stream, &version);

	if(version != PXI_VERSION) {
		log_error("Bad version %u; expected %u", version, PXI_VERSION);
		return false;
	}

	uint32_t crc32 = PXI_CRC32_INIT;
	cstream = NOT_NULL(SDL_RWWrapCRC32(stream, &crc32, false));

	// TODO validate and verify consistency of data_size/width/height/format

	SDL_ReadU32LE(cstream, &pixmap->width);
	SDL_ReadU32LE(cstream, &pixmap->height);
	SDL_ReadU32LE(cstream, &pixmap->data_size);

	uint16_t tmp_u16 = 0;
	uint8_t tmp_u8 = 0;

	SDL_ReadU16LE(cstream, &tmp_u16);
	pixmap->format = tmp_u16;

	SDL_ReadU8(cstream, &tmp_u8);
	pixmap->origin = tmp_u8;

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

	pixmap->data.untyped = mem_alloc(pixmap->data_size);
	size_t read = SDL_ReadIO(cstream, pixmap->data.untyped, pixmap->data_size);

	if(read == 0 && SDL_GetIOStatus(cstream) != SDL_IO_STATUS_EOF) {
		log_sdl_error(LOG_ERROR, "SDL_ReadIO");
		goto fail;
	}

	if(read != pixmap->data_size) {
		log_error("Read %zu bytes, expected %u", read, pixmap->data_size);
		goto fail;
	}

	SDL_CloseIO(cstream);
	cstream = NULL;

	uint32_t crc32_in = 0;
	SDL_ReadU32LE(stream, &crc32_in);

	if(crc32_in != crc32) {
		log_error("CRC32 mismatch: 0x%08x != 0x%08x", crc32_in, crc32);
		goto fail;
	}

	return true;

fail:
	if(pixmap->data.untyped) {
		mem_free(pixmap->data.untyped);
		pixmap->data.untyped = NULL;
	}

	if(cstream) {
		SDL_CloseIO(cstream);
	}

	return false;
}

PixmapFileFormatHandler pixmap_fileformat_internal = {
	.probe = px_internal_probe,
	.save = px_internal_save,
	.load = px_internal_load,
	.filename_exts = (const char*[]) { NULL },
	.name = "Taisei internal",
};
