/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_pixmap_serialize_h
#define IGUARD_pixmap_serialize_h

#include "taisei.h"

#include "pixmap.h"

enum {
	PIXMAP_SERIALIZED_VERSION = 1,
};

bool pixmap_serialize(SDL_RWops *stream, const Pixmap *pixmap);
bool pixmap_deserialize(SDL_RWops *stream, Pixmap *pixmap);

#endif // IGUARD_pixmap_serialize_h
