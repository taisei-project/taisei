/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rwops_stdiofp.h"
#include <SDL3/SDL_error.h>

#include <stdio.h>

// NOTE: Copypasted from https://github.com/libsdl-org/SDL/blob/0aa1022358e09e6bb22a1087c51a10c954a8ab18/docs/README-migration.md

typedef struct IOStreamStdioFPData {
	FILE *fp;
	bool autoclose;
} IOStreamStdioFPData;

static Sint64 SDLCALL stdio_seek(void *userdata, Sint64 offset, SDL_IOWhence whence)
{
	FILE *fp = ((IOStreamStdioFPData *) userdata)->fp;
	int stdiowhence;

	switch (whence) {
		case SDL_IO_SEEK_SET:
			stdiowhence = SEEK_SET;
			break;
		case SDL_IO_SEEK_CUR:
			stdiowhence = SEEK_CUR;
			break;
		case SDL_IO_SEEK_END:
			stdiowhence = SEEK_END;
			break;
		default:
			return SDL_SetError("Unknown value for 'whence'");
	}

	if (fseek(fp, offset, stdiowhence) == 0) {
		const Sint64 pos = ftell(fp);
		if (pos < 0) {
			return SDL_SetError("Couldn't get stream offset");
		}
		return pos;
	}
	return SDL_SetError("Seek error");
}

static size_t SDLCALL stdio_read(void *userdata, void *ptr, size_t size, SDL_IOStatus *status)
{
	FILE *fp = ((IOStreamStdioFPData *) userdata)->fp;
	const size_t bytes = fread(ptr, 1, size, fp);
	if (bytes == 0 && ferror(fp)) {
		*status = SDL_IO_STATUS_ERROR;
		SDL_SetError("Read error");
	}
	return bytes;
}

static size_t SDLCALL stdio_write(void *userdata, const void *ptr, size_t size, SDL_IOStatus *status)
{
	FILE *fp = ((IOStreamStdioFPData *) userdata)->fp;
	const size_t bytes = fwrite(ptr, 1, size, fp);
	if (bytes == 0 && ferror(fp)) {
		*status = SDL_IO_STATUS_ERROR;
		SDL_SetError("Write error");
	}
	return bytes;
}

static bool SDLCALL stdio_close(void *userdata)
{
	IOStreamStdioFPData *rwopsdata = (IOStreamStdioFPData *) userdata;
	bool status = true;
	if (rwopsdata->autoclose) {
		if (fclose(rwopsdata->fp) != 0) {
			status = SDL_SetError("Write error");
		}
	}
	SDL_free(rwopsdata);
	return status;
}

SDL_IOStream *SDL_RWFromFP(FILE *fp, bool autoclose)
{
	SDL_IOStreamInterface iface;
	IOStreamStdioFPData *rwopsdata;
	SDL_IOStream *rwops;

	rwopsdata = (IOStreamStdioFPData *) SDL_malloc(sizeof (*rwopsdata));
	if (!rwopsdata) {
		return NULL;
	}

	SDL_INIT_INTERFACE(&iface);
	/* There's no stdio_size because SDL_GetIOSize emulates it the same way we'd do it for stdio anyhow. */
	iface.seek = stdio_seek;
	iface.read = stdio_read;
	iface.write = stdio_write;
	iface.close = stdio_close;

	rwopsdata->fp = fp;
	rwopsdata->autoclose = autoclose;

	rwops = SDL_OpenIO(&iface, rwopsdata);
	if (!rwops) {
		iface.close(rwopsdata);
	}

	SDL_PropertiesID props = SDL_GetIOProperties(rwops);
	assume(props != 0);
	SDL_SetPointerProperty(props, SDL_PROP_IOSTREAM_STDIO_FILE_POINTER, fp);

	return rwops;
}
