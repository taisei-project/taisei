/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "util.h"
#include "fileformats.h"

#include <webp/decode.h>

static bool px_webp_probe(SDL_RWops *stream) {
	// "RIFF", file size (4 bytes), "WEBP", ("VP8 " | "VP8L" | "VP8X")
	// https://developers.google.com/speed/webp/docs/riff_container

	uchar header[16] = { 0 };
	SDL_RWread(stream, header, sizeof(header), 1);

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

static bool px_webp_load(SDL_RWops *stream, Pixmap *pixmap, PixmapFormat preferred_format) {
	WebPDecoderConfig config;
	int status = WebPInitDecoderConfig(&config);

	if(!status) {
		log_error("Library ABI mismatch");
		return false;
	}

	// NOTE: We read the image upside down here to avoid needing to flip it for
	// the GL backend. This is just a slight optimization, not a hard dependency.
	// config.options.flip = true;
	// pixmap->origin = PIXMAP_ORIGIN_BOTTOMLEFT;
	// BUG: Unfortunately, the decoder seems to ignore the flip option, at least in
	// some instances.

	// TODO: Make sure this isn't counter-productive with our own inter-resource
	// loading parallelism.
	config.options.use_threads = true;

	// TODO: Investigate the effect of dithering on image quality and decoding
	// time. Only relevant for lossy images.
	// config.options.dithering_strength = 50;
	// config.options.alpha_dithering_strength = 50;

	WebPBitstreamFeatures features;
	uint8_t buf[BUFSIZ] = { 0 };
	size_t data_available = SDL_RWread(stream, buf, 1, sizeof(buf));

	status = WebPGetFeatures(buf, data_available, &features);

	if(status != VP8_STATUS_OK) {
		log_error("WebPGetFeatures() failed: %s", webp_error_str(status));
		return false;
	}

	WebPIDecoder *idec = WebPINewDecoder(&config.output);

	if(idec == NULL) {
		log_error("WebPINewDecoder() failed");
		return false;
	}

	pixmap->width = features.width;
	pixmap->height = features.height;
	pixmap->format = features.has_alpha ? PIXMAP_FORMAT_RGBA8 : PIXMAP_FORMAT_RGB8;

	size_t pixel_size = PIXMAP_FORMAT_PIXEL_SIZE(pixmap->format);

	if(pixmap->height > PIXMAP_BUFFER_MAX_SIZE / (pixmap->width * pixel_size)) {
		WebPIDelete(idec);
		log_error("The image is too large");
		return false;
	}

	pixmap->data.untyped = pixmap_alloc_buffer_for_copy(pixmap, &pixmap->data_size);

	config.output.is_external_memory = true;
	config.output.colorspace = features.has_alpha ? MODE_RGBA : MODE_RGB;
	config.output.u.RGBA.rgba = pixmap->data.untyped;
	config.output.u.RGBA.size = pixmap->data_size;
	config.output.u.RGBA.stride = pixel_size * pixmap->width;

	do {
		status = WebPIAppend(idec, buf, data_available);

		if(status != VP8_STATUS_OK && status != VP8_STATUS_SUSPENDED) {
			log_error("WebPIAppend() failed: %s", webp_error_str(status));
			mem_free(pixmap->data.untyped);
			pixmap->data.untyped = NULL;
			break;
		}

		data_available = SDL_RWread(stream, buf, 1, sizeof(buf));
	} while(data_available > 0);

	WebPIDelete(idec);
	WebPFreeDecBuffer(&config.output);
	return pixmap->data.untyped != NULL;
}

PixmapFileFormatHandler pixmap_fileformat_webp = {
	.probe = px_webp_probe,
	.load = px_webp_load,
	.filename_exts = (const char*[]) { "webp", NULL },
	.name = "WebP",
};
