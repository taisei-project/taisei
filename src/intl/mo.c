/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "intl.h"
#include "vfs/public.h"
#include <SDL3/SDL_iostream.h>

#include "mo.h"

typedef struct MoReadContext {
	const char *filename;
	SDL_IOStream *stream;

	uint32_t number_of_strings;
	uint32_t original_strings_offset;
	uint32_t translated_strings_offset;

	char **original_strings;
	IntlTranslation *translated_strings;
} MoReadContext;

static bool read_mo_header(MoReadContext *ctx) {
	uint32_t magic;
	if(!SDL_ReadU32LE(ctx->stream, &magic)) {
		log_error("%s: Error reading magic bytes: %s", ctx->filename, SDL_GetError());
		return false;
	}

	if(magic != 0x950412de) {
		log_error("%s is not a little-endian MO file: Magic header does not match.", ctx->filename);
	}
	uint32_t revision;
	if(!SDL_ReadU32LE(ctx->stream, &revision)) {
		log_error("%s: Error reading MO number of strings: %s", ctx->filename, SDL_GetError());
		return false;
	}
	if(revision != 0) {
		log_error("%s: Unsupported MO version", ctx->filename);
	}

	if(!SDL_ReadU32LE(ctx->stream, &ctx->number_of_strings)) {
		log_error("%s: Error reading MO number of strings: %s", ctx->filename, SDL_GetError());
		return false;
	}
	ctx->number_of_strings--; // skip metadata string
	if(!SDL_ReadU32LE(ctx->stream, &ctx->original_strings_offset)) {
		log_error("%s: Error reading MO original strings offset: %s", ctx->filename, SDL_GetError());
		return false;
	}
	if(!SDL_ReadU32LE(ctx->stream, &ctx->translated_strings_offset)) {
		log_error("%s: Error reading MO translated strings offset: %s", ctx->filename, SDL_GetError());
		return false;
	}

	return true;
}

static bool read_mo_string(MoReadContext *ctx, char **dest, uint32_t *length) {
	if(!SDL_ReadU32LE(ctx->stream, length)) {
		log_error("%s: Error reading MO string length: %s", ctx->filename, SDL_GetError());
		return false;
	}
	uint32_t offset;
	if(!SDL_ReadU32LE(ctx->stream, &offset)) {
		log_error("%s: Error reading MO string offset: %s", ctx->filename, SDL_GetError());
		return false;
	}

	int64_t current_offset = SDL_TellIO(ctx->stream);

	if(!SDL_SeekIO(ctx->stream, offset, SDL_IO_SEEK_SET)) {
		log_error("%s: Error seeking to MO string: %s", ctx->filename, SDL_GetError());
		return false;
	}

	*dest = mem_alloc(*length+1);
	if(!SDL_ReadIO(ctx->stream, *dest, *length+1)) {
		log_error("%s: Error reading MO string content: %s", ctx->filename, SDL_GetError());
		return false;
	}

	if(!SDL_SeekIO(ctx->stream, current_offset, SDL_IO_SEEK_SET)) {
		log_error("%s: Error seeking MO: %s", ctx->filename, SDL_GetError());
		return false;
	}
	return true;
}

IntlTextDomain *intl_read_mo(const char* path) {
	MoReadContext context = { .filename = path };
	context.stream = vfs_open(path, VFS_MODE_READ | VFS_MODE_SEEKABLE);
	if(!context.stream) {
		log_error("VFS error: %s", vfs_get_error());
		return NULL;
	}

	if(!read_mo_header(&context)) {
		return NULL;
	}

	IntlTextDomain *domain = NULL;

	context.original_strings = mem_alloc_array(context.number_of_strings, sizeof(char *));
	context.translated_strings = mem_alloc_array(context.number_of_strings, sizeof(IntlTranslation));

	int read_original_strings = 0;
	int read_translated_strings = 0;

	// + 8 skips metadata string
	SDL_SeekIO(context.stream, context.original_strings_offset + 8, SDL_IO_SEEK_SET);

	for(int i = 0; i < context.number_of_strings; i++) {
		uint32_t length;
		if(!read_mo_string(&context, &context.original_strings[i], &length)) {
			goto cleanup;
		}
		read_original_strings++;
	}

	SDL_SeekIO(context.stream, context.translated_strings_offset + 8, SDL_IO_SEEK_SET);
	for(int i = 0; i < context.number_of_strings; i++) {
		uint32_t length;
		char *text;
		if(!read_mo_string(&context, &text, &length)) {
			goto cleanup;
		}
		intl_translation_create(&context.translated_strings[i], text, length);
		mem_free(text);
		read_translated_strings++;
	}

	domain = intl_textdomain_new(context.number_of_strings, context.original_strings, context.translated_strings);

cleanup:
	for(int i = 0; i < read_original_strings; i++) {
		mem_free(context.original_strings[i]);
	}
	mem_free(context.original_strings);
	for(int i = 0; i < read_translated_strings; i++) {
		intl_translation_destroy(&context.translated_strings[i]);
	}
	mem_free(context.translated_strings);

	return domain;
}

