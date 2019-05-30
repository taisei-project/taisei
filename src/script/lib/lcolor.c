/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "color.h"
#include "all.h"
#include <lauxlib.h>

#define COLOR_NAME "Color"

static bool lcolor_rgb_chanvalue(lua_State *L, int idx, int chan, float *outval) {
	if(chan == 4 && lua_isnoneornil(L, idx)) {
		// alpha unspecified? default to 1
		*outval = 1;
		return true;
	}

	int isnum;
	lua_Number val = lua_tonumberx(L, idx, &isnum);

	if(!l_numisreal(val)) {
		isnum = false;
	}

	if(isnum) {
		*outval = val;
		return true;
	}

	return false;
}

static Color *lcolor_new(lua_State *L, const Color *initval) {
	Color *clr = lua_newuserdata(L, sizeof(Color));
	*clr = *initval;
	luaL_setmetatable(L, COLOR_NAME);
	return clr;
}

static Color *lcolor_seqtocolor(lua_State *L, int idx) {
	Color c;

	for(int i = 1; i <= 4; ++i) {
		lua_geti(L, idx, i);

		if(!lcolor_rgb_chanvalue(L, -1, i, c.array + i - 1)) {
			lua_pushfstring(L, "channel %i value is not a real number", i);
			luaL_argerror(L, idx, lua_tostring(L, -1));
			UNREACHABLE;
		}

		lua_pop(L, 1);
	}

	Color *clr = lcolor_new(L, &c);
	lua_replace(L, idx);
	return clr;
}

Color *lcolor_tocolor(lua_State *L, int idx, bool copy) {
	Color *clr = luaL_testudata(L, idx, COLOR_NAME);

	if(clr) {
		if(copy) {
			clr = lcolor_new(L, clr);
			lua_replace(L, idx);
		}
		return clr;
	}

	return lcolor_seqtocolor(L, idx);
}

static int lcolor_rgb(lua_State *L) {
	luaL_checkany(L, 1);

	Color *clr = luaL_testudata(L, 1, COLOR_NAME);

	if(clr) {
		lcolor_new(L, clr);
		return 1;
	}

	Color c;
	int isnum;
	c.r = lua_tonumberx(L, 1, &isnum);

	if(isnum) {
		luaL_checkrealnumber(L, 1);
		c.g = luaL_checkrealnumber(L, 2);
		c.b = luaL_checkrealnumber(L, 3);
		c.a = luaL_optrealnumber(L, 4, 1);
		lcolor_new(L, &c);
		return 1;
	}

	lcolor_tocolor(L, 1, false);
	return 1;
}

static int lcolor_hsl(lua_State *L) {
	lcolor_rgb(L);
	Color *clr = lua_touserdata(L, -1);
	assert(luaL_testudata(L, -1, COLOR_NAME));
	color_hsla(clr, clr->r, clr->g, clr->b, clr->a);
	return 1;
}

static int lcolor_call(lua_State *L) {
	lua_remove(L, 1);
	return lcolor_rgb(L);
}

static bool parseswizzle(lua_State *L, int idx, schar indices[4], int *numelems) {
	int isnum;
	lua_Integer chan = lua_tointegerx(L, 2, &isnum);

	if(isnum) {
		if(chan > 0 && chan <= 4) {
			*numelems = 1;
			indices[0] = chan - 1;
			return true;
		}

		return false;
	}

	size_t slen;
	const char *schan = lua_tolstring(L, 2, &slen);

	if(!schan || slen < 1 || slen > 4) {
		return false;
	}

	for(int i = 0; i < slen; ++i) {
		switch(schan[i]) {
			case 'r': indices[i] = 0; break;
			case 'g': indices[i] = 1; break;
			case 'b': indices[i] = 2; break;
			case 'a': indices[i] = 3; break;
			default: return false;
		}
	}

	*numelems = slen;
	return true;
}

static int lcolor_swizzleget(lua_State *L) {
	Color *clr = luaL_checkudata(L, 1, COLOR_NAME);
	schar swizzle[4];
	int numelems;

	if(!parseswizzle(L, 2, swizzle, &numelems)) {
		int type = lua_type(L, 2);

		if(type == LUA_TSTRING) {
			attr_unused int got_metatable = lua_getmetatable(L, 1);
			assert(got_metatable && "color has no metatable?");

			lua_pushvalue(L, 2);
			lua_rawget(L, -2);

			if(lua_isnoneornil(L, -1) || *lua_tostring(L, 2) == '_') {
				lua_pushfstring(L, "'%s' is neither a "COLOR_NAME" method nor a valid swizzle mask", lua_tostring(L, 2));
				luaL_argerror(L, 2, lua_tostring(L, -1));
			}

			return 1;
		} else if(type == LUA_TNUMBER) {
			// somewhat inconsistent, but allows treating color just like a sequence
			return 0;
		}

		luaL_typeerror(L, 2, "integer or string");
		return 0;
	}

	if(numelems > 1) {
		lua_createtable(L, numelems, 0);
		for(int i = 0; i < numelems; ++i) {
			lua_pushnumber(L, clr->array[swizzle[i]]);
			lua_rawseti(L, -2, i + 1);
		}
	} else {
		lua_pushnumber(L, clr->array[swizzle[0]]);
	}

	return 1;
}

static int lcolor_swizzleset(lua_State *L) {
	Color *clr = luaL_checkudata(L, 1, COLOR_NAME);
	schar swizzle[4];
	int numelems;

	if(!parseswizzle(L, 2, swizzle, &numelems)) {
		lua_pushfstring(L, "'%s' is not a valid swizzle mask", luaL_tolstring(L, 2, NULL));
		luaL_argerror(L, 2, lua_tostring(L, -1));
	}

	if(numelems == 1) {
		clr->array[swizzle[0]] = luaL_checkrealnumber(L, 3);
	} else {
		for(int i = 0; i < numelems; ++i) {
			lua_geti(L, 3, i + 1);
			clr->array[swizzle[i]] = luaL_checkrealnumber(L, -1);
			lua_pop(L, 1);
		}
	}

	return 0;
}

static int lcolor_len(lua_State *L) {
	lua_pushinteger(L, 4);
	return 1;
}

static int lcolor_eq(lua_State *L) {
	Color *clr1 = lcolor_tocolor(L, 1, false);
	Color *clr2 = lcolor_tocolor(L, 2, false);
	lua_pushboolean(L, color_equals(clr1, clr2));
	return 1;
}

static int lcolor_add_aux(lua_State *L, bool copy) {
	Color *clr1 = lcolor_tocolor(L, 1, copy);
	Color *clr2 = lcolor_tocolor(L, 2, false);
	color_add(clr1, clr2);
	lua_settop(L, 1);
	return 1;
}

static int lcolor_add_nocopy(lua_State *L) { return lcolor_add_aux(L, false); }
static int lcolor_add_copy(lua_State *L) { return lcolor_add_aux(L, true); }

static int lcolor_sub_aux(lua_State *L, bool copy) {
	Color *clr1 = lcolor_tocolor(L, 1, copy);
	Color *clr2 = lcolor_tocolor(L, 2, false);
	color_sub(clr1, clr2);
	lua_settop(L, 1);
	return 1;
}

static int lcolor_sub_nocopy(lua_State *L) { return lcolor_sub_aux(L, false); }
static int lcolor_sub_copy(lua_State *L) { return lcolor_sub_aux(L, true); }

static int lcolor_mul_aux(lua_State *L, bool copy) {
	Color *clr1 = lcolor_tocolor(L, 1, copy);
	int is_scalar;
	lua_RealNumber scalar = lua_tonumberx(L, 2, &is_scalar);

	if(is_scalar) {
		color_mul_scalar(clr1, scalar);
	} else {
		Color *clr2 = lcolor_tocolor(L, 2, false);
		color_mul(clr1, clr2);
	}

	lua_settop(L, 1);
	return 1;
}

static int lcolor_mul_nocopy(lua_State *L) { return lcolor_mul_aux(L, false); }
static int lcolor_mul_copy(lua_State *L) { return lcolor_mul_aux(L, true); }

static int lcolor_mul_alpha(lua_State *L) {
	Color *clr = lcolor_tocolor(L, 1, false);
	color_mul_alpha(clr);
	lua_settop(L, 1);
	return 1;
}

static int lcolor_div_aux(lua_State *L, bool copy) {
	Color *clr1 = lcolor_tocolor(L, 1, copy);
	int is_scalar;
	lua_RealNumber scalar = lua_tonumberx(L, 2, &is_scalar);

	if(is_scalar) {
		color_div_scalar(clr1, scalar);
	} else {
		Color *clr2 = lcolor_tocolor(L, 2, false);
		color_div(clr1, clr2);
	}

	lua_settop(L, 1);
	return 1;
}

static int lcolor_div_nocopy(lua_State *L) { return lcolor_div_aux(L, false); }
static int lcolor_div_copy(lua_State *L) { return lcolor_div_aux(L, true); }

static int lcolor_div_alpha(lua_State *L) {
	Color *clr = lcolor_tocolor(L, 1, false);
	color_div_alpha(clr);
	lua_settop(L, 1);
	return 1;
}

static int lcolor_pow_aux(lua_State *L, bool copy) {
	Color *clr1 = lcolor_tocolor(L, 1, copy);
	int is_scalar;
	lua_RealNumber scalar = lua_tonumberx(L, 2, &is_scalar);

	if(is_scalar) {
		color_pow_scalar(clr1, scalar);
	} else {
		Color *clr2 = lcolor_tocolor(L, 2, false);
		color_pow(clr1, clr2);
	}

	lua_settop(L, 1);
	return 1;
}

static int lcolor_pow_nocopy(lua_State *L) { return lcolor_pow_aux(L, false); }
static int lcolor_pow_copy(lua_State *L) { return lcolor_pow_aux(L, true); }

static int lcolor_lerp(lua_State *L) {
	Color *clr1 = lcolor_tocolor(L, 1, false);
	Color *clr2 = lcolor_tocolor(L, 2, false);
	lua_RealNumber a = luaL_checkrealnumber(L, 3);
	color_lerp(clr1, clr2, a);
	lua_settop(L, 1);
	return 1;
}

static int lcolor_unm(lua_State *L) {
	Color *clr = lcolor_tocolor(L, 1, false);
	clr->r = -clr->r;
	clr->g = -clr->g;
	clr->b = -clr->b;
	clr->a = -clr->a;
	lua_settop(L, 1);
	return 1;
}

static int lcolor_tostring(lua_State *L) {
	Color *clr = luaL_checkudata(L, 1, COLOR_NAME);
	lua_pushfstring(L, COLOR_NAME"(%f, %f, %f, %f): %p",
		// CAUTION the casts are important
		(LUAI_UACNUMBER)clr->r,
		(LUAI_UACNUMBER)clr->g,
		(LUAI_UACNUMBER)clr->b,
		(LUAI_UACNUMBER)clr->a,
		clr
	);
	return 1;
}

LUAMOD_API int luaopen_color(lua_State *L) {
	luaL_newmetatable(L, COLOR_NAME);
	luaL_setfuncs(L, ((luaL_Reg[]) {
		{ "__eq", lcolor_eq },
		{ "__index", lcolor_swizzleget },
		{ "__len", lcolor_len },
		{ "__newindex", lcolor_swizzleset },
		{ "__tostring", lcolor_tostring },
		{ "__unm", lcolor_unm },
		{ "copy", lcolor_rgb },
		{ "lerp", lcolor_lerp },
		{ "__add", lcolor_add_copy },
		{ "add", lcolor_add_nocopy },
		{ "__sub", lcolor_sub_copy },
		{ "sub", lcolor_sub_nocopy },
		{ "__mul", lcolor_mul_copy },
		{ "mul", lcolor_mul_nocopy },
		{ "mulalpha", lcolor_mul_alpha },
		{ "__div", lcolor_div_copy },
		{ "div", lcolor_div_nocopy },
		{ "divalpha", lcolor_div_alpha },
		{ "__pow", lcolor_pow_copy },
		{ "pow", lcolor_pow_nocopy },
		{ NULL },
	}), 0);

	luaL_newlib(L, ((luaL_Reg[]) {
		{ "rgb", lcolor_rgb },
		{ "hsl", lcolor_hsl },
		{ "add", lcolor_add_nocopy },
		{ "sub", lcolor_sub_nocopy },
		{ "mul", lcolor_mul_nocopy },
		{ "mulalpha", lcolor_mul_alpha },
		{ "div", lcolor_div_nocopy },
		{ "divalpha", lcolor_div_alpha },
		{ "pow", lcolor_pow_nocopy },
		{ "lerp", lcolor_lerp },
		{ NULL },
	}));
	luaL_newlib(L, ((luaL_Reg[]) {
		{ "__call", lcolor_call },
		{ NULL },
	}));
	lua_setmetatable(L, -2);

	return 1;
}
