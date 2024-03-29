/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "rwops_stdiofp.h"

#include <stdio.h>

// NOTE: Copypasted from https://github.com/libsdl-org/SDL/blob/0aa1022358e09e6bb22a1087c51a10c954a8ab18/docs/README-migration.md

typedef struct IOStreamStdioFPData {
	FILE *fp;
	SDL_bool autoclose;
} IOStreamStdioFPData;

static Sint64 SDLCALL stdio_seek(void *userdata, Sint64 offset, int whence)
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
	return SDL_Error(SDL_EFSEEK);
}

static size_t SDLCALL stdio_read(void *userdata, void *ptr, size_t size, SDL_IOStatus *status)
{
	FILE *fp = ((IOStreamStdioFPData *) userdata)->fp;
	const size_t bytes = fread(ptr, 1, size, fp);
	if (bytes == 0 && ferror(fp)) {
		SDL_Error(SDL_EFREAD);
	}
	return bytes;
}

static size_t SDLCALL stdio_write(void *userdata, const void *ptr, size_t size, SDL_IOStatus *status)
{
	FILE *fp = ((IOStreamStdioFPData *) userdata)->fp;
	const size_t bytes = fwrite(ptr, 1, size, fp);
	if (bytes == 0 && ferror(fp)) {
		SDL_Error(SDL_EFWRITE);
	}
	return bytes;
}

static int SDLCALL stdio_close(void *userdata)
{
	IOStreamStdioFPData *rwopsdata = (IOStreamStdioFPData *) userdata;
	int status = 0;
	if (rwopsdata->autoclose) {
		if (fclose(rwopsdata->fp) != 0) {
			status = SDL_Error(SDL_EFWRITE);
		}
	}
	return status;
}

SDL_IOStream *SDL_RWFromFP(FILE *fp, SDL_bool autoclose)
{
	SDL_IOStreamInterface iface;
	IOStreamStdioFPData *rwopsdata;
	SDL_IOStream *rwops;

	rwopsdata = (IOStreamStdioFPData *) SDL_malloc(sizeof (*rwopsdata));
	if (!rwopsdata) {
		return NULL;
	}

	SDL_zero(iface);
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
	return rwops;
}
