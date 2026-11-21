/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util/kvparser.h"  // IWYU pragma: export

bool parse_keyvalue_file_cb(const char *filename, KVCallback callback, void *data);
bool parse_keyvalue_file(const char *filename, ht_str2ptr_t *hashtable);
bool parse_keyvalue_file_with_spec(const char *filename, KVSpec *spec);
