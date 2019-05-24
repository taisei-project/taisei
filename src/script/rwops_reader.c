/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <SDL_rwops.h>
#include <lua.h>

#include "rwops_reader.h"

struct LuaRWopsReader {
	SDL_RWops *rwops;
	char buffer[1024];
	bool autoclose;
};

static const char *lrwreader_read(lua_State *L, void *data, size_t *size) {
	LuaRWopsReader *lrw = data;
	*size = SDL_RWread(lrw->rwops, lrw->buffer, 1, sizeof(lrw->buffer));
	return lrw->buffer;
}

LuaRWopsReader *lrwreader_create(SDL_RWops *rwops, bool autoclose, lua_Reader *out_readfunc) {
	LuaRWopsReader *lrw = calloc(1, sizeof(*lrw));
	lrw->rwops = rwops;
	lrw->autoclose = autoclose;
	*out_readfunc = lrwreader_read;
	return lrw;
}

void lrwreader_destroy(LuaRWopsReader *lrw) {
	if(lrw->autoclose) {
		SDL_RWclose(lrw->rwops);
	}

	free(lrw);
}
