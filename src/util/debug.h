/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#ifdef DEBUG
	typedef struct DebugInfo {
		const char *file;
		const char *func;
		uint line;
	} DebugInfo;

	#define _DEBUG_INFO_INITIALIZER_ { __FILE__, __func__, __LINE__ }
	#define _DEBUG_INFO_ ((DebugInfo) _DEBUG_INFO_INITIALIZER_)
	#define _DEBUG_INFO_PTR_ (&_DEBUG_INFO_)
	#define set_debug_info(debug) _set_debug_info(debug, _DEBUG_INFO_PTR_)
	void _set_debug_info(DebugInfo *debug, DebugInfo *meta);
	DebugInfo* get_debug_info(void);
	DebugInfo* get_debug_meta(void);

	extern bool _in_draw_code;
	#define BEGIN_DRAW_CODE() do { if(_in_draw_code) { log_fatal("BEGIN_DRAW_CODE not followed by END_DRAW_CODE"); } _in_draw_code = true; } while(0)
	#define END_DRAW_CODE() do { if(!_in_draw_code) { log_fatal("END_DRAW_CODE not preceeded by BEGIN_DRAW_CODE"); } _in_draw_code = false; } while(0)
	#define IN_DRAW_CODE (_in_draw_code)
#else
	#define set_debug_info(debug)
	#define BEGIN_DRAW_CODE()
	#define END_DRAW_CODE()
	#define IN_DRAW_CODE (0)
#endif
