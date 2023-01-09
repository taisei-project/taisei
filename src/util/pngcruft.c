/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "pngcruft.h"
#include "log.h"

static void pngutil_rwops_write_data(png_structp png_ptr, png_bytep data, png_size_t length) {
	SDL_RWops *out = png_get_io_ptr(png_ptr);
	SDL_RWwrite(out, data, length, 1);
}

static void pngutil_rwops_flush_data(png_structp png_ptr) {
	// no flush operation in SDL_RWops
}

static void pngutil_rwops_read_data(png_structp png_ptr, png_bytep data, png_size_t length) {
	SDL_RWops *out = png_get_io_ptr(png_ptr);
	SDL_RWread(out, data, length, 1);
}

void pngutil_init_rwops_read(png_structp png, SDL_RWops *rwops) {
	png_set_read_fn(png, rwops, pngutil_rwops_read_data);
}

void pngutil_init_rwops_write(png_structp png, SDL_RWops *rwops) {
	png_set_write_fn(png, rwops, pngutil_rwops_write_data, pngutil_rwops_flush_data);
}

noreturn static void pngutil_error_handler(png_structp png_ptr, png_const_charp error_msg) {
	log_error("PNG error: %s", error_msg);
	png_longjmp(png_ptr, 1);
}

static void pngutil_warning_handler(png_structp png_ptr, png_const_charp warning_msg) {
	log_warn("PNG warning: %s", warning_msg);
}

void pngutil_setup_error_handlers(png_structp png) {
	png_set_error_fn(png, NULL, pngutil_error_handler, pngutil_warning_handler);
}

static PNG_CALLBACK(png_voidp, pngutil_malloc, (png_structp png, png_alloc_size_t size)) {
	return mem_alloc(size);
}

static PNG_CALLBACK(void, pngutil_free, (png_structp png, png_voidp ptr)) {
	mem_free(ptr);
}

png_structp pngutil_create_read_struct(void) {
	return png_create_read_struct_2(
		PNG_LIBPNG_VER_STRING,
		NULL, pngutil_error_handler, pngutil_warning_handler,
		NULL, pngutil_malloc, pngutil_free);
}

png_structp pngutil_create_write_struct(void) {
	return png_create_write_struct_2(
		PNG_LIBPNG_VER_STRING,
		NULL, pngutil_error_handler, pngutil_warning_handler,
		NULL, pngutil_malloc, pngutil_free);
}
