/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "taisei_err.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// void err(int eval, const char *fmt, ...);
void errx(int eval, const char *fmt, ...) {
	va_list ap;

	char *buf = malloc(11+strlen(fmt));
	strcpy(buf, "!- ERROR: ");
	strcat(buf, fmt);
	strcat(buf, "\n");

	va_start(ap, fmt);
	vprintf(buf, ap);
	va_end(ap);
	free(buf);

	exit(eval);
}

// void warn(const char *fmt, ...);
void warnx(const char *fmt, ...) {
	va_list ap;

	char *buf = malloc(14+strlen(fmt));
	strcpy(buf, "!- WARNING: ");
	strcat(buf, fmt);
	strcat(buf, "\n");

	va_start(ap, fmt);
	vprintf(buf, ap);
	va_end(ap);
	free(buf);
}