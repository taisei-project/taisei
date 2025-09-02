/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
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

size_t taisei_version_read(SDL_IOStream *rwops, TaiseiVersion *version) {
	size_t read_size = 0;

	if(!SDL_ReadU8(rwops, &version->major)) return read_size;
	read_size += sizeof(version->major);
	if(!SDL_ReadU8(rwops, &version->minor)) return read_size;
	read_size += sizeof(version->minor);
	if(!SDL_ReadU8(rwops, &version->patch)) return read_size;
	read_size += sizeof(version->patch);
	if(!SDL_ReadU16LE(rwops, &version->tweak)) return read_size;
	read_size += sizeof(version->tweak);

	assume(TAISEI_VERSION_SIZE == read_size);
	return TAISEI_VERSION_SIZE;
}

size_t taisei_version_write(SDL_IOStream *rwops, TaiseiVersion *version) {
	size_t write_size = 0;

	if(!SDL_WriteU8(rwops, version->major)) return write_size;
	write_size += sizeof(version->major);
	if(!SDL_WriteU8(rwops, version->minor)) return write_size;
	write_size += sizeof(version->minor);
	if(!SDL_WriteU8(rwops, version->patch)) return write_size;
	write_size += sizeof(version->patch);
	if(!SDL_WriteU16LE(rwops, version->tweak)) return write_size;
	write_size += sizeof(version->tweak);

	assert(write_size == TAISEI_VERSION_SIZE);
	return write_size;
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
