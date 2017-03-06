/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "taisei_err.h"
#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "global.h"

// void err(int eval, const char *fmt, ...);
void errx(int eval, const char *fmt, ...) {
	va_list ap;
	char buf[1024];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	fprintf(stderr, "!- ERROR: %s\n", buf);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Taisei error", buf, NULL);

	exit(eval);
}

// void warn(const char *fmt, ...);
void warnx(const char *fmt, ...) {
	va_list ap;

	char *buf = strjoin("!- WARNING: ", fmt, "\n", NULL);

	va_start(ap, fmt);
	vfprintf(stderr, buf, ap);
	va_end(ap);
	free(buf);
}
