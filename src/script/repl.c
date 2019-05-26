/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "repl.h"
#include "console.h"
#include "util.h"
#include "script_internal.h"

static struct {
	char *chunk_buffer;
	size_t chunk_len;
} repl;

static void push_partial_chunk(const char *chunk) {
	size_t part_len = strlen(chunk);
	repl.chunk_buffer = realloc(repl.chunk_buffer, repl.chunk_len + part_len + 2);
	memcpy(repl.chunk_buffer + repl.chunk_len, chunk, part_len);
	repl.chunk_len += part_len;
	repl.chunk_buffer[repl.chunk_len++] = '\n';
	repl.chunk_buffer[repl.chunk_len] = 0;
}

static int try_compile(lua_State *L) {
	const char *retline = lua_pushfstring(L, "return %s;", repl.chunk_buffer);
	int status = luaL_loadbufferx(L, retline, repl.chunk_len + sizeof("return ;") - 1, "=(console)", "t");

	if(status == LUA_OK) {
		// remove the modified string from the stack
		lua_remove(L, -2);
	} else {
		// remove the modified string and the luaL_loadbufferx result from the stack
		lua_pop(L, 2);
		status = luaL_loadbufferx(L, repl.chunk_buffer, repl.chunk_len, "=(console)", "t");
	}

	return status;
}

ScriptReplResult script_repl(const char *chunk) {
	ScriptReplResult result = REPL_OK;
	assert(lua_gettop(script.lstate) == 0);

	push_partial_chunk(chunk);

	if(try_compile(script.lstate) != LUA_OK) {
		const char *err = lua_tostring(script.lstate, -1);

		if(strendswith(err, " near <eof>")) {
			result = REPL_NEEDMORE;
		} else {
			con_printf("%s\n", err);
			result = REPL_ERROR;
		}

		lua_pop(script.lstate, 1);
	} else if(script_pcall_with_msghandler(script.lstate, 0, LUA_MULTRET) != LUA_OK) {
		const char *err = lua_tostring(script.lstate, -1);
		con_printf("%s\n", err);
		result = REPL_ERROR;
		lua_pop(script.lstate, 1);
	}

	con_set_prompt(result == REPL_NEEDMORE ? ">> " : "> ");

	// print all values returned by the chunk
	int n = lua_gettop(script.lstate);

	if(n > 0) {
		luaL_checkstack(script.lstate, LUA_MINSTACK, "too many results to print");
		lua_getglobal(script.lstate, "print");
		lua_insert(script.lstate, 1);

		if(lua_pcall(script.lstate, n, 0, 0) != LUA_OK) {
			con_printf("error calling 'print' (%s)\n", lua_tostring(script.lstate, -1));
		}

		lua_settop(script.lstate, 0);
	}

	if(result != REPL_NEEDMORE) {
		script_repl_abort();  // free chunk buffer
	}

	return result;
}

void script_repl_abort(void) {
	free(repl.chunk_buffer);
	repl.chunk_buffer = NULL;
	repl.chunk_len = 0;
}
