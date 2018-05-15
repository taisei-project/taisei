/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "crap.h"

#include <stdlib.h>
#include <string.h>

void* memdup(const void *src, size_t size) {
	void *data = malloc(size);
	memcpy(data, src, size);
	return data;
}
