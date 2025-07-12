/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "fileformats.h"
#include "log.h"

#include <webp/decode.h>

static bool px_webp_probe(SDL_IOStream *stream) {
	// "RIFF", file size (4 bytes), "WEBP", ("VP8 " | "VP8L" | "VP8X")
	// https://developers.google.com/speed/webp/docs/riff_container

	uchar header[16] = { 0 };
	SDL_ReadIO(stream, header, sizeof(header));

	return (
		!memcmp(header, "RIFF", 4) &&
		!memcmp(header + 8, "WEBP", 4) && (
			!memcmp(header + 12, "VP8 ", 4) || // simple lossy
			!memcmp(header + 12, "VP8L", 4) || // simple lossless
			!memcmp(header + 12, "VP8X", 4)    // extended
		)
	);
}

static inline const char* webp_error_str(VP8StatusCode code)  {
	static const char* map[] = {
		[VP8_STATUS_OK] = "No error",
		[VP8_STATUS_OUT_OF_MEMORY] = "Out of memory",
		[VP8_STATUS_INVALID_PARAM] = "Invalid parameter",
		[VP8_STATUS_BITSTREAM_ERROR] = "Bitstream error",
		[VP8_STATUS_UNSUPPORTED_FEATURE] = "Unsupported feature",
		[VP8_STATUS_SUSPENDED] = "Suspended",
		[VP8_STATUS_USER_ABORT] = "Aborted by user",
		[VP8_STATUS_NOT_ENOUGH_DATA] = "Not enough data",
	};

	if((uint)code < sizeof(map)/sizeof(*map)) {
		return map[code];
	}

	return "Unknown error";
}

static bool px_webp_load(SDL_IOStream *stream, Pixmap *pixmap, PixmapFormat preferred_format) {
	size_t webp_bufsize;
	uint8_t *webp_buffer = SDL_LoadFile_IO(stream, &webp_bufsize, false);

	if(UNLIKELY(!webp_buffer)) {
		log_sdl_error(LOG_ERROR, "SDL_LoadFile_IO");
		return false;
	}

	WebPBitstreamFeatures features;
	VP8StatusCode status = WebPGetFeatures(webp_buffer, webp_bufsize, &features);

	if(UNLIKELY(status != VP8_STATUS_OK)) {
		SDL_free(webp_buffer);
		log_error("WebPGetFeatures() failed: %s", webp_error_str(status));
		return false;
	}

	pixmap->width = features.width;
	pixmap->height = features.height;

	if(!features.has_alpha || preferred_format == PIXMAP_FORMAT_RGB8) {
		pixmap->format = PIXMAP_FORMAT_RGB8;
	} else {
		pixmap->format = PIXMAP_FORMAT_RGBA8;
	}

	// TODO: add a way to indicate preference
	pixmap->origin = PIXMAP_ORIGIN_BOTTOMLEFT;

	size_t pixel_size = PIXMAP_FORMAT_PIXEL_SIZE(pixmap->format);
	size_t scanline_size = pixel_size * pixmap->width;

	if(UNLIKELY(pixmap->height > PIXMAP_BUFFER_MAX_SIZE / scanline_size)) {
		SDL_free(webp_buffer);
		log_error("The image is too large");
		return false;
	}

	pixmap->data.untyped = pixmap_alloc_buffer_for_copy(pixmap, &pixmap->data_size);
	bool ok = false;

	int stride;
	uint8_t *begin;

	if(pixmap->origin == PIXMAP_ORIGIN_BOTTOMLEFT) {
		stride = -(int)scanline_size;
		begin = (uint8_t*)pixmap->data.untyped + pixmap->data_size - scanline_size;
	} else {
		stride = scanline_size;
		begin = pixmap->data.untyped;
	}

	if(pixmap->format == PIXMAP_FORMAT_RGBA8) {
		if(UNLIKELY(!(ok = WebPDecodeRGBAInto(
			webp_buffer, webp_bufsize, begin, pixmap->data_size, stride))
		)) {
			log_error("WebPDecodeRGBAInto() failed");
		}
	} else {
		if(UNLIKELY(!(ok = WebPDecodeRGBInto(
			webp_buffer, webp_bufsize, begin, pixmap->data_size, stride))
		)) {
			log_error("WebPDecodeRGBInto() failed");
		}
	}

	SDL_free(webp_buffer);

	if(UNLIKELY(!ok)) {
		mem_free(pixmap->data.untyped);
		pixmap->data.untyped = NULL;
	}

	return ok;
}

PixmapFileFormatHandler pixmap_fileformat_webp = {
	.probe = px_webp_probe,
	.load = px_webp_load,
	.filename_exts = (const char*[]) { "webp", NULL },
	.name = "WebP",
};
