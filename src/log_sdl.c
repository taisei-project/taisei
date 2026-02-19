/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "log_sdl.h"

#include "log.h"
#include "util/env.h"

static SDLCALL void sdl_log(void *userdata, int category, SDL_LogPriority priority, const char *message) {
	const char *cat_str, *prio_str;
	LogLevel lvl = LOG_DEBUG;

	switch(category) {
		case SDL_LOG_CATEGORY_APPLICATION: cat_str = "Application"; break;
		case SDL_LOG_CATEGORY_ERROR:       cat_str = "Error"; break;
		case SDL_LOG_CATEGORY_ASSERT:      cat_str = "Assert"; break;
		case SDL_LOG_CATEGORY_SYSTEM:      cat_str = "System"; break;
		case SDL_LOG_CATEGORY_AUDIO:       cat_str = "Audio"; break;
		case SDL_LOG_CATEGORY_VIDEO:       cat_str = "Video"; break;
		case SDL_LOG_CATEGORY_RENDER:      cat_str = "Render"; break;
		case SDL_LOG_CATEGORY_INPUT:       cat_str = "Input"; break;
		case SDL_LOG_CATEGORY_TEST:        cat_str = "Test"; break;
		case SDL_LOG_CATEGORY_GPU:         cat_str = "GPU"; break;
		default:                           cat_str = "Unknown"; break;
	}

	switch(priority) {
		case SDL_LOG_PRIORITY_VERBOSE:  prio_str = "Verbose"; break;
		case SDL_LOG_PRIORITY_DEBUG:    prio_str = "Debug"; break;
		case SDL_LOG_PRIORITY_INFO:     prio_str = "Info"; lvl = LOG_INFO; break;
		case SDL_LOG_PRIORITY_WARN:     prio_str = "Warn"; lvl = LOG_WARN; break;
		case SDL_LOG_PRIORITY_ERROR:    prio_str = "Error"; lvl = LOG_ERROR; break;
		case SDL_LOG_PRIORITY_CRITICAL: prio_str = "Critical"; lvl = LOG_ERROR; break;
		default:                        prio_str = "Unknown"; break;
	}

	log_custom(lvl, "[%s, %s] %s", cat_str, prio_str, message);
}

void log_sdl_init(SDL_LogPriority prio) {
	prio = env_get("TAISEI_SDL_LOG", prio);
	SDL_SetLogPriorities(prio);
	SDL_SetLogOutputFunction(sdl_log, NULL);
}
