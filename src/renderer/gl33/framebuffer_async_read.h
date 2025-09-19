/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"

void gl33_framebuffer_read_async(
	Framebuffer *framebuffer,
	FramebufferAttachment attachment,
	IntRect region,
	void *userdata,
	FramebufferReadAsyncCallback callback
);

void gl33_framebuffer_process_read_requests(void);
void gl33_framebuffer_finalize_read_requests(void);
