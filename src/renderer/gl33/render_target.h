/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "../api.h"
#include "opengl.h"
#include "texture.h"

struct RenderTargetImpl {
	GLuint gl_fbo;
	bool initialized;
	Texture *attachments[RENDERTARGET_MAX_ATTACHMENTS];
};

void gl33_target_initialize(RenderTarget *target);

void gl33_target_create(RenderTarget *target);
void gl33_target_attach(RenderTarget *target, Texture *tex, RenderTargetAttachment attachment);
Texture* gl33_target_get_attachment(RenderTarget *target, RenderTargetAttachment attachment);
void gl33_target_destroy(RenderTarget *target);
