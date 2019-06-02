
#include "all.h"
#include "util.h"
#include "vfs/pathutil.h"
#include "../rwops_reader.h"

static const char *getmodname(lua_State *L, int idx, size_t *out_len) {
	size_t slen;
	const char *rawmodname = luaL_checklstring(L, idx, &slen);

	if(slen > 255) {
		lua_pushstring(L, "require: module name too long");
		lua_error(L);
	}

	char buf[slen + 1];
	char *pbuf = buf;

	for(const char *c = rawmodname; *c;) {
		if(*c == '.') {
			*pbuf++ = *c++;
			if(*c == 0 || *c == '.') {
				lua_pushfstring(L, "require: malformed module name '%s'", rawmodname);
				lua_error(L);
			}
		} else if(!isalnum(*c) && *c != '_') {
			lua_pushfstring(L, "require: malformed module name '%s'", rawmodname);
			lua_error(L);
		} else {
			*pbuf++ = *c++;
		}
	}

	*pbuf = 0;

	if(*buf == '.') {
		lua_Debug ar;

		for(int i = 1; lua_getstack(L, i, &ar); ++i) {
			lua_getinfo(L, "St", &ar);

			if(ar.istailcall) {
				break;
			}

			if(ar.source && ar.source[0] == '@') {
				if(!strstartswith(ar.source + 1, SCRIPT_PATH_PREFIX VFS_PATH_SEPARATOR_STR)) {
					lua_pushfstring(L, "require: bad chunk name in caller (%s)", ar.source);
					lua_error(L);
				}

				const char *srcptr = ar.source + sizeof(SCRIPT_PATH_PREFIX VFS_PATH_SEPARATOR_STR);
				size_t srclen = strlen(srcptr);
				char fullname[srclen + strlen(buf) + 2];
				vfs_path_resolve_relative(fullname, sizeof(fullname), srcptr, buf + 1);
				for(char *c; (c = strchr(fullname, VFS_PATH_SEPARATOR));) {
					*c = '.';
				}

				lua_pushstring(L, fullname);
				lua_replace(L, idx);
				return lua_tolstring(L, idx, out_len);
			}
		}

		lua_pushstring(L, "require: can't perform relative import in this context");
		lua_error(L);
	}

	lua_pushstring(L, buf);
	lua_replace(L, idx);
	return lua_tolstring(L, idx, out_len);
}

static int lbaseext_require(lua_State *L) {
	size_t modnamelen;
	const char *modname = getmodname(L, 1, &modnamelen);

	luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
	lua_getfield(L, -1, modname);

	if(lua_toboolean(L, -1)) {
		// already loaded? just return it.
		return 1;
	}

	lua_pop(L, 1);
	const size_t psize = sizeof(SCRIPT_PATH_PREFIX VFS_PATH_SEPARATOR_STR) - 1;
	char buf[sizeof(SCRIPT_PATH_PREFIX) + modnamelen + 7];
	memcpy(buf + 1, SCRIPT_PATH_PREFIX VFS_PATH_SEPARATOR_STR, psize);
	memcpy(buf + 1 + psize, modname, modnamelen + 1);
	for(char *c = buf + 1 + psize; (c = strchr(c, '.'));) {
		*c = VFS_PATH_SEPARATOR;
	}
	memcpy(buf + 1 + psize + modnamelen, ".lua", 5);
	buf[0] = '@';

	log_debug("Trying to load new module '%s'...", modname);

	SDL_RWops *rw = vfs_open(buf + 1, VFS_MODE_READ);

	if(!rw) {
		lua_pushfstring(L, "VFS error: %s", vfs_get_error());
		lua_error(L);
	}

	lua_Reader reader;
	LuaRWopsReader *lrw = lrwreader_create(rw, true, &reader);

	if(lua_load(L, reader, lrw, buf, "t") != LUA_OK) {
		lrwreader_destroy(lrw);
		lua_error(L);
	}

	lrwreader_destroy(lrw);

	lua_call(L, 0, 1);
	if(!lua_toboolean(L, -1)) {
		lua_pop(L, 1);
		lua_pushboolean(L, true);
	}
	lua_pushvalue(L, -1);
	lua_setfield(L, -3, modname);

	log_debug("Loaded new module '%s'", modname);
	return 1;
}

void lapi_baseext_register(lua_State *L) {
	lua_register(L, "require", lbaseext_require);
}
