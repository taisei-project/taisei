/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "assert.h"
#include "util.h"

void _ts_assert_fail(const char *cond, const char *func, const char *file, int line, bool use_log) {
    use_log = use_log && log_initialized();

    if(use_log) {
        log_fatal("%s:%i: %s(): assertion `%s` failed", file, line, func, cond);
    } else {
        tsfprintf(stderr, "%s:%i: %s(): assertion `%s` failed", file, line, func, cond);
        abort();
    }
}
