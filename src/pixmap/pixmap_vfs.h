/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "pixmap.h"

bool pixmap_load_file(const char *path, Pixmap *dst, PixmapFormat preferred_format) attr_nonnull(1, 2) attr_nodiscard;

bool pixmap_save_file(const char *path, const Pixmap *src, const PixmapSaveOptions *opts) attr_nonnull(1, 2);

char *pixmap_source_path(const char *prefix, const char *path) attr_nodiscard;
