/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "env.h"

#include "log.h"

const char *env_get_string(const char *var, const char *fallback) {
	const char *val = SDL_GetEnvironmentVariable(SDL_GetEnvironment(), var);

	if(val == NULL) {
		return fallback;
	}

	return val;
}

const char *env_get_string_nonempty(const char *var, const char *fallback) {
	const char *val = SDL_GetEnvironmentVariable(SDL_GetEnvironment(), var);

	if(!val || !*val) {
		return fallback;
	}

	return val;
}

void env_set_string(const char *var, const char *val, bool override) {
	log_debug("%s=%s (%i)", var, val, override);
	SDL_SetEnvironmentVariable(SDL_GetEnvironment(), var, val, override);
}

int64_t env_get_int(const char *var, int64_t fallback) {
	const char *val = SDL_GetEnvironmentVariable(SDL_GetEnvironment(), var);

	if(val == NULL) {
		return fallback;
	}

	return strtoll(val, NULL, 10);
}

void env_set_int(const char *var, int64_t val, bool override) {
	char buf[21];
	snprintf(buf, sizeof(buf), "%"PRIi64, val);
	env_set_string(var, buf, override);
}

double env_get_double(const char *var, double fallback) {
	const char* val = SDL_GetEnvironmentVariable(SDL_GetEnvironment(), var);

	if(val == NULL) {
		return fallback;
	}

	return strtod(val, NULL);
}

void env_set_double(const char *var, double val, bool override) {
	char buf[24];
	snprintf(buf, sizeof(buf), "%.14g", val);
	env_set_string(var, buf, override);
}
