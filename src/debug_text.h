/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

/*
 * Generic on-screen debug text facility
 *
 * Call debug_text(format, ...) from anywhere to add a line of text to display in the top left corner of the screen.
 * All debug text is hidden by default. See FILTERING below for how to selectively enable the ones you want to see.
 * For immediate quick-and-dirty debugging, use debug_text_raw(format, ...), which is always displayed, but please do
 * not commit that into the repo.
 *
 * debug_text() and debug_text_raw() calls are only enabled in developer builds, or if ENABLE_DEBUG_TEXT is explicitly
 * defined. The arguments are not guaranteed to be evaluated and should not have any side effects.
 *
 * *** FILTERING ***
 *
 * All debug_text() calls are tagged by the file and function they are used in. The tag format is module:function, where
 * module is constructed the same way as in the log. Filters are simple glob expressions that match on the tag, with an
 * optional - prefix to negate the match. Filters are loaded from two places, in order:
 * 	1. The `storage/debug_text_filter` file, if it exists
 *	2. The TAISEI_DEBUG_TEXT environment variable
 *
 * Filters are separated by any of these: spaces, tabs, commas, semicolons, linebreaks.
 * Later filters take precedence over earlier ones.
 *
 * Any debug_text() that is reached but gets filtered out is logged once; grep the log for debug_text_check_filter
 * to see the missed tags.
 */
// Example filters:
//
//      1. Enable everything (not recommended):
//         *
//
//      2. Disable everything (can be used to override an earlier filter):
//         -*
//
//      3. Enable everything in renderer/:
//         renderer/*
//
//      4. Enable my_cool_func in any module:
//         *:my_cool_func
//
//      5. Enable everything in renderer/, except gl33, but keep gl33_cool_func:
//         renderer/* -renderer/gl33* renderer/gl33*:gl33_cool_func
//

#if DEBUG
#define ENABLE_DEBUG_TEXT
#endif

#ifdef ENABLE_DEBUG_TEXT

void debug_text_init(void);
void debug_text_shutdown(void);

// These are called by the eventloop executor
// "commit" marks a save point: when "rollback" is used, every line added after the last
// "commit" is deleted. This is used to allow the executor to run multiple logic frames
// in a single tick, keeping only the last one's debug lines, while preserving any debug
// lines the executor might've added itself before running logic.
// NOTE: this is not a stack!
void debug_text_rollback(void);
void debug_text_commit(void);
void debug_text_clear(void);
void debug_text_draw(void);

void debug_text_raw(const char *fmt, ...) attr_printf(1, 2);
bool debug_text_check_filter(const char *file, const char *func, int line);

// This is what you actually want to use
#define debug_text(fmt, ...) ({ \
	static int _enabled = -1; \
	if(_enabled == -1) _enabled = debug_text_check_filter(_TAISEI_SRC_FILE, __func__, __LINE__); \
	if(_enabled) debug_text_raw(fmt, ##__VA_ARGS__); \
	(void)0; \
})

#else	// ENABLE_DEBUG_TEXT

#define debug_text_init()      do {} while(0)
#define debug_text_shutdown()  do {} while(0)
#define debug_text_rollback()  do {} while(0)
#define debug_text_commit()    do {} while(0)
#define debug_text_clear()     do {} while(0)
#define debug_text_draw()      do {} while(0)

attr_printf(1, 2) INLINE void _debug_text_nop(const char *fmt, ...) { }
#define debug_text(fmt, ...)   do { if(0) _debug_text_nop(fmt, ##__VA_ARGS__); } while(0)

// debug_text_raw intentionally omitted so that it fails CI if it gets committed by accident

#endif  // ENABLE_DEBUG_TEXT
