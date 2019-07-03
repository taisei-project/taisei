/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "opuscruft.h"

#include <SDL.h>
#include <opusfile.h>

static int opusutil_rwops_read(void *_stream, unsigned char *_ptr, int _nbytes) {
	SDL_RWops *rw = _stream;
	return SDL_RWread(rw, _ptr, 1, _nbytes);
}

static int opusutil_rwops_seek(void *_stream, opus_int64 _offset, int _whence) {
	SDL_RWops *rw = _stream;
	return SDL_RWseek(rw, _offset, _whence) < 0 ? -1 : 0;
}

static opus_int64 opusutil_rwops_tell(void *_stream) {
	SDL_RWops *rw = _stream;
	return SDL_RWtell(rw);
}

static int opusutil_rwops_close(void *_stream) {
	SDL_RWops *rw = _stream;
	return SDL_RWclose(rw) < 0 ? EOF : 0;
}

OpusFileCallbacks opusutil_rwops_callbacks = {
	.read = opusutil_rwops_read,
	.seek = opusutil_rwops_seek,
	.tell = opusutil_rwops_tell,
	.close = opusutil_rwops_close,
};
