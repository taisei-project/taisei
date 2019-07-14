/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_resource_bgm_metadata_h
#define IGUARD_resource_bgm_metadata_h

#include "taisei.h"

#include "resource.h"

typedef struct MusicMetadata {
	RESOURCE_ALIGN
	char *title;
	char *artist;
	char *comment;
	char *loop_path;
	double loop_point;
} MusicMetadata;

extern ResourceHandler bgm_metadata_res_handler;

#endif // IGUARD_resource_bgm_metadata_h
