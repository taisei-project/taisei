/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stringops.h"
#include "miscmath.h"
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

#ifndef TAISEI_BUILDCONF_HAVE_STRTOK_R
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
char *strtok_r(char *str, const char *delim, char **nextp) {
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
#endif

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

void format_huge_num(uint digits, uint64_t num, size_t bufsize, char buf[bufsize]) {
	if(digits == 0) {
		digits = digitcnt(num);
	}

	num = umin(upow10(digits) - 1, num);

	div_t separators = div(digits, 3);
	attr_unused uint len = digits + (separators.quot + !!separators.rem);
	assert(bufsize >= len);

	char *p = buf;

	// WARNING: i must be signed here
	for(int i = 0; i < digits; ++i) {
		if(i && !((i - separators.rem) % 3)) {
			*p++ = ',';
		}

		uint64_t divisor = upow10(digits - i - 1);
		*p++ = '0' + num / divisor;
		num %= divisor;
	}

	*p = 0;
	assert(p == buf + len - 1);
}

void hexdigest(uint8_t *input, size_t input_size, char *output, size_t output_size) {
	assert(output_size > input_size * 2);

	static char charmap[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
	uint8_t *end = input + input_size;

	while(input < end) {
		uint8_t byte = *input++;
		*output++ = charmap[byte >> 4];
		*output++ = charmap[byte & 0xf];
	}

	*output = 0;
}

void expand_escape_sequences(char *str) {
	bool in_escape = false;
	char *p = str;

	for(p = str; *p; ++p) {
		if(in_escape) {
			switch(*p) {
				case 'n': p[-1] = '\n'; break;
				case 't': p[-1] = '\t'; break;
				case 'r': p[-1] = '\r'; break;
				default:  p[-1] =   *p; break;
			}

			memmove(p, p + 1, strlen(p + 1) + 1);
			--p;
			in_escape = false;
		} else if(*p == '\\') {
			in_escape = true;
		}
	}

	if(in_escape) {
		p[-1] = 0;
	}
}

#ifndef TAISEI_BUILDCONF_HAVE_MEMRCHR
void *memrchr(const void *s, int c, size_t n) {
	const char *mem = s;

	for(const char *p = mem + n - 1; p >= mem; --p) {
		if(*p == c) {
			return (void*)p;
		}
	}

	return NULL;
}
#endif
