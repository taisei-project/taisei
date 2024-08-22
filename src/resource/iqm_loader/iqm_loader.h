/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "renderer/api.h"

typedef struct IQMModel {
	GenericModelVertex *vertices;
	uint32_t *indices;
	uint num_vertices;
	uint num_indices;
} IQMModel;

bool iqm_load_stream(IQMModel *model, const char *filename, SDL_IOStream *stream);
