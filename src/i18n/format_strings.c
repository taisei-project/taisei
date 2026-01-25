/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "format_strings.h"

static uint8_t flag_bits[] = {
	['+'] = 0x01,
	['-'] = 0x02,
	[' '] = 0x04,
	['\''] = 0x08,
	['#'] = 0x10,
	['0'] = 0x20,
	['I'] = 0x40,
};

static int count_format_specifiers(const char *s) {
	int count = 0;

	for(const char *p = s; *p; p++) {
		if(*p == '%') {
			if(*(p+1) == '%') {
				p++;
			} else {
				count++;
			}
		}
	}
	return count;
}

int is_format_specifier(const char *s) {
	const char *p = s;
	if(*p != '%') {
		return 0;
	}
	p++;
	if(*p == '%') {
		return 0;
	}

	// argument
	while((*p >= '0' && *p <= '9') || *p == '$') {
		p++;
	}
	// flag
	uint8_t flags = 0x0;
	while(*p == '+' || *p == '-' || *p == ' ' || *p == '\'' || *p == '#' || *p == '0' || *p == 'I') {
		if(flags & flag_bits[(uint8_t)*p]) {
			return 0; // no duplicate flags
		}
		flags |= flag_bits[(uint8_t)*p];
		p++;
	}

	// precision
	while((*p >= '0' && *p <= '9') || *p == '*' || *p == '.' || *p == '$') {
		p++;
	}

	// length modifier
	while(*p == 'h' || *p == 'l' || *p == 'q' || *p == 'j' || *p == 'z' || *p == 'Z' || *p == 't' || *p == 'L') {
		p++;
	}
	// conversion specifier
	if((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) {
		return p + 1 - s;
	}
	return 0;
}

bool match_format_strings(const char *a, const char *b) {
	int count = count_format_specifiers(a);
	int match_count = 0;
	while(*a || *b) {
		int lengtha = 0;
		while(*a && !(lengtha = is_format_specifier(a))) {
			if(*a == '%' && *(a+1) == '%') {
				a++;
			}
			a++;
		}
		int lengthb = 0;
		while(*b && !(lengthb = is_format_specifier(b))) {
			if(*b == '%' && *(b+1) == '%') {
				b++;
			}
			b++;
		}

		if(lengtha != lengthb) {
			return false;
		}

		if(strncmp(a, b, lengtha) != 0) {
			return false;
		}

		match_count += lengtha != 0;

		a += lengtha;
		b += lengthb;
	}

	if(match_count != count) {
		assert(!"Format specifier parser failed to find specifier. This is either a bug or we are using an unexpected format string feature.");
		return false;
	}
	return true;
}
