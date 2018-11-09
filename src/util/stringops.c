/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <string.h>
#include <stdlib.h>

#include "stringops.h"
#include "assert.h"

bool strendswith(const char *s, const char *e) {
	int ls = strlen(s);
	int le = strlen(e);

	if(le > ls)
		return false;

	int i; for(i = 0; i < le; ++i)
	if(s[ls-i-1] != e[le-i-1])
		return false;

	return true;
}

bool strstartswith(const char *s, const char *p) {
	int ls = strlen(s);
	int lp = strlen(p);

	if(ls < lp)
		return false;

	return !strncmp(s, p, lp);
}

bool strendswith_any(const char *s, const char **earray) {
	for(const char **e = earray; *e; ++e) {
		if(strendswith(s, *e)) {
			return true;
		}
	}

	return false;
}

bool strstartswith_any(const char *s, const char **earray) {
	for(const char **e = earray; *e; ++e) {
		if(strstartswith(s, *e)) {
			return true;
		}
	}

	return false;
}

void stralloc(char **dest, const char *src) {
	free(*dest);

	if(src) {
		*dest = malloc(strlen(src)+1);
		strcpy(*dest, src);
	} else {
		*dest = NULL;
	}
}

char* strjoin(const char *first, ...) {
	va_list args;
	size_t size = strlen(first) + 1;
	char *str = malloc(size);

	strcpy(str, first);
	va_start(args, first);

	for(;;) {
		char *next = va_arg(args, char*);

		if(!next) {
			break;
		}

		size += strlen(next);
		str = realloc(str, size);
		strcat(str, next);
	}

	va_end(args);
	return str;
}

char* vstrfmt(const char *fmt, va_list args) {
	size_t written = 0;
	size_t fmtlen = strlen(fmt);
	size_t asize = 1;
	char *out = NULL;

	while(asize <= fmtlen)
		asize *= 2;

	for(;;) {
		out = realloc(out, asize);

		va_list nargs;
		va_copy(nargs, args);
		written = vsnprintf(out, asize, fmt, nargs);
		va_end(nargs);

		if(written < asize) {
			break;
		}

		asize = written + 1;
	}

	if(asize > strlen(out) + 1) {
		out = realloc(out, strlen(out) + 1);
	}

	return out;
}

char* strfmt(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char *str = vstrfmt(fmt, args);
	va_end(args);
	return str;
}

char* strftimealloc(const char *fmt, const struct tm *timeinfo) {
	size_t sz_allocated = 64;

	while(true) {
		char str[sz_allocated];

		if(strftime(str, sz_allocated, fmt, timeinfo)) {
			return strdup(str);
		}

		sz_allocated *= 2;
	};
}

char* copy_segment(const char *text, const char *delim, int *size) {
	char *seg, *beg, *end;

	beg = strstr(text, delim);
	if(!beg)
		return NULL;
	beg += strlen(delim);

	end = strstr(beg, "%%");
	if(!end)
		return NULL;

	*size = end-beg;
	seg = malloc(*size+1);
	strlcpy(seg, beg, *size+1);

	return seg;
}

void strip_trailing_slashes(char *buf) {
	for(char *c = buf + strlen(buf) - 1; c >= buf && (*c == '/' || *c == '\\'); c--)
		*c = 0;
}

char* strappend(char **dst, char *src) {
	if(!*dst) {
		return *dst = strdup(src);
	}

	*dst = realloc(*dst, strlen(*dst) + strlen(src) + 1);
	strcat(*dst, src);
	return *dst;
}


#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	#define UCS4_ID "UCS-4BE"
#else
	#define UCS4_ID "UCS-4LE"
#endif

uint32_t* ucs4chr(const uint32_t *ucs4, uint32_t chr) {
	for(; *ucs4 != chr; ++ucs4) {
		if(!*ucs4) {
			return NULL;
		}
	}

	return (uint32_t*)ucs4;
}

size_t ucs4len(const uint32_t *ucs4) {
	size_t len;
	for(len = 0; *ucs4; ++len, ++ucs4);
	return len;
}

void utf8_to_ucs4(const char *utf8, size_t bufsize, uint32_t buf[bufsize]) {
	uint32_t *bufptr = buf;
	assert(bufsize > 0);

	while(*utf8) {
		*bufptr++ = utf8_getch(&utf8);
		assert(bufptr < buf + bufsize);
	}

	*bufptr++ = 0;
}

uint32_t* utf8_to_ucs4_alloc(const char *utf8) {
	uint32_t *ucs4 = (uint32_t*)(void*)SDL_iconv_string(UCS4_ID, "UTF-8", utf8, strlen(utf8) + 1);
	assert(ucs4 != NULL);
	return ucs4;
}

void ucs4_to_utf8(const uint32_t *ucs4, size_t bufsize, char buf[bufsize]) {
	assert(bufsize > ucs4len(ucs4));
	char *temp = ucs4_to_utf8_alloc(ucs4);
	memcpy(buf, temp, bufsize);
	free(temp);
}

char* ucs4_to_utf8_alloc(const uint32_t *ucs4) {
	char *utf8 = SDL_iconv_string("UTF-8", UCS4_ID, (const char*)ucs4, sizeof(uint32_t) * (ucs4len(ucs4) + 1));
	assert(utf8 != NULL);
	return utf8;
}

/*
 * public domain strtok_r() by Charlie Gordon
 *
 *   from comp.lang.c  9/14/2007
 *
 *      http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *
 *     (Declaration that it's public domain):
 *      http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 */
char* strtok_r(char *str, const char *delim, char **nextp) {
	char *ret;

	if(str == NULL) {
		str = *nextp;
	}

	str += strspn(str, delim);

	if(*str == '\0') {
		return NULL;
	}

	ret = str;
	str += strcspn(str, delim);

	if(*str) {
		*str++ = '\0';
	}

	*nextp = str;
	return ret;
}

static uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t crc32str(uint32_t crc, const char *str) {
	const uint8_t *p = (uint8_t*)str;
	crc = crc ^ ~0U;

	while(*p) {
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
	}

	return crc ^ ~0U;
}

size_t filename_timestamp(char *buf, size_t buf_size, SystemTime systime) {
	assert(buf_size >= FILENAME_TIMESTAMP_MIN_BUF_SIZE);

	char buf_msecs[4];
	char buf_datetime[FILENAME_TIMESTAMP_MIN_BUF_SIZE - sizeof(buf_msecs)];
	attr_unused size_t written;

	written = snprintf(buf_msecs, sizeof(buf_msecs), "%03u", (uint)(systime.tv_nsec / 1000000));
	assert(written == sizeof(buf_msecs) - 1);
	written = strftime(buf_datetime, sizeof(buf_datetime), "%Y%m%d_%H-%M-%S", localtime(&systime.tv_sec));
	assert(written != 0);

	return snprintf(buf, buf_size, "%s-%s", buf_datetime, buf_msecs);
}

uint32_t utf8_getch(const char **src) {
	// Ported from SDL_ttf and slightly modified

	assert(*src != NULL);

	const uint8_t *p = *(const uint8_t**)src;
	size_t left = 0;
	bool overlong = false;
	uint32_t ch = UNICODE_UNKNOWN;

	if(**src == 0) {
		return UNICODE_UNKNOWN;
	}

	if(p[0] >= 0xFC) {
		if((p[0] & 0xFE) == 0xFC) {
			if(p[0] == 0xFC && (p[1] & 0xFC) == 0x80) {
				overlong = true;
			}
			ch = p[0] & 0x01;
			left = 5;
		}
	} else if(p[0] >= 0xF8) {
		if((p[0] & 0xFC) == 0xF8) {
			if(p[0] == 0xF8 && (p[1] & 0xF8) == 0x80) {
				overlong = true;
			}
			ch = p[0] & 0x03;
			left = 4;
		}
	} else if(p[0] >= 0xF0) {
		if((p[0] & 0xF8) == 0xF0) {
			if(p[0] == 0xF0 && (p[1] & 0xF0) == 0x80) {
				overlong = true;
			}
			ch = p[0] & 0x07;
			left = 3;
		}
	} else if(p[0] >= 0xE0) {
		if((p[0] & 0xF0) == 0xE0) {
			if(p[0] == 0xE0 && (p[1] & 0xE0) == 0x80) {
				overlong = true;
			}
			ch = p[0] & 0x0F;
			left = 2;
		}
	} else if(p[0] >= 0xC0) {
		if((p[0] & 0xE0) == 0xC0) {
			if((p[0] & 0xDE) == 0xC0) {
				overlong = true;
			}
			ch = p[0] & 0x1F;
			left = 1;
		}
	} else {
		if((p[0] & 0x80) == 0x00) {
			ch = p[0];
		}
	}

	++*src;

	while(left > 0 && **src != 0) {
		++p;

		if((p[0] & 0xC0) != 0x80) {
			ch = UNICODE_UNKNOWN;
			break;
		}

		ch <<= 6;
		ch |= (p[0] & 0x3F);

		++*src;
		--left;
	}

	if(
		overlong ||
		left > 0 ||
		(ch >= 0xD800 && ch <= 0xDFFF) ||
		(ch == 0xFFFE || ch == 0xFFFF) ||
		ch > 0x10FFFF
	) {
		ch = UNICODE_UNKNOWN;
	}

	return ch;
}
