/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "version.h"

int taisei_version_compare(TaiseiVersion *v1, TaiseiVersion *v2, TaiseiVersionCmpLevel level) {
	int result = 0;

	if((result = (int)v1->major - (int)v2->major) && level >= VCMP_MAJOR) return result;
	if((result = (int)v1->minor - (int)v2->minor) && level >= VCMP_MINOR) return result;
	if((result = (int)v1->patch - (int)v2->patch) && level >= VCMP_PATCH) return result;
	if((result = (int)v1->tweak - (int)v2->tweak) && level >= VCMP_TWEAK) return result;

	return result;
}

size_t taisei_version_read(SDL_RWops *rwops, TaiseiVersion *version) {
	// XXX: detect errors somehow?

	version->major = SDL_ReadU8(rwops);
	version->minor = SDL_ReadU8(rwops);
	version->patch = SDL_ReadU8(rwops);
	version->tweak = SDL_ReadLE16(rwops);

	return TAISEI_VERSION_SIZE;
}

size_t taisei_version_write(SDL_RWops *rwops, TaiseiVersion *version) {
	size_t wrote_now = 0, wrote_total = 0;

	if(!(wrote_now = SDL_WriteU8(rwops, version->major))) {
		return wrote_total;
	} else {
		wrote_total += wrote_now;
	}

	if(!(wrote_now = SDL_WriteU8(rwops, version->minor))) {
		return wrote_total;
	} else {
		wrote_total += wrote_now;
	}

	if(!(wrote_now = SDL_WriteU8(rwops, version->patch))) {
		return wrote_total;
	} else {
		wrote_total += wrote_now;
	}

	if(!(wrote_now = 2 * SDL_WriteLE16(rwops, version->tweak))) {
		return wrote_total;
	} else {
		wrote_total += wrote_now;
	}

	assert(wrote_total == TAISEI_VERSION_SIZE);
	return wrote_total;
}

char *taisei_version_tostring(TaiseiVersion *version) {
	StringBuffer sbuf = {};
	taisei_version_tostrbuf(&sbuf, version);
	return sbuf.start;
}

void taisei_version_tostrbuf(StringBuffer *sbuf, TaiseiVersion *version) {
	strbuf_printf(sbuf, "%u.%u", version->major, version->minor);

	if(!version->patch && !version->tweak) {
		return;
	}

	strbuf_printf(sbuf, ".%u", version->patch);

	if(!version->tweak) {
		return;
	}

	strbuf_printf(sbuf, ".%u", version->tweak);
}
