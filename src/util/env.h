/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

const char *env_get_string(const char *var, const char *fallback)
	attr_nonnull(1);

const char *env_get_string_nonempty(const char *var, const char *fallback)
	attr_nonnull(1);

void env_set_string(const char *var, const char *val, bool override)
	attr_nonnull(1, 2);

int64_t env_get_int(const char *var, int64_t fallback)
	attr_nonnull(1);

void env_set_int(const char *var, int64_t val, bool override)
	attr_nonnull(1);

double env_get_double(const char *var, double fallback)
	attr_nonnull(1);

void env_set_double(const char *var, double val, bool override)
	attr_nonnull(1);

#define env_get(var, fallback) (_Generic((fallback), \
		const char* : env_get_string, \
		char*       : env_get_string, \
		void*       : env_get_string, \
		bool        : env_get_int, \
		int8_t      : env_get_int, \
		uint8_t     : env_get_int, \
		int16_t     : env_get_int, \
		uint16_t    : env_get_int, \
		int32_t     : env_get_int, \
		uint32_t    : env_get_int, \
		int64_t     : env_get_int, \
		uint64_t    : env_get_int, \
		double      : env_get_double, \
		float       : env_get_double \
	)(var, fallback))

#define env_set(var, val, override) (_Generic((val), \
		const char* : env_set_string, \
		char*       : env_set_string, \
		void*       : env_set_string, \
		bool        : env_set_int, \
		int8_t      : env_set_int, \
		uint8_t     : env_set_int, \
		int16_t     : env_set_int, \
		uint16_t    : env_set_int, \
		int32_t     : env_set_int, \
		uint32_t    : env_set_int, \
		int64_t     : env_set_int, \
		uint64_t    : env_set_int, \
		double      : env_set_double, \
		float       : env_set_double \
	)(var, val, override))
