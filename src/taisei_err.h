/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

/* replacement for standard err.h, some environments don't have it ... but i like it :/ */

#include <stdarg.h>

// void err(int eval, const char *fmt, ...);
void errx(int eval, const char *fmt, ...);

// void warn(const char *fmt, ...);
void warnx(const char *fmt, ...);
