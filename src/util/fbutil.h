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

typedef struct FBAttachmentConfig {
	FramebufferAttachment attachment;
	TextureParams tex_params;
} FBAttachmentConfig;

void fbutil_create_attachments(Framebuffer *fb, uint num_attachments, FBAttachmentConfig attachments[num_attachments]);
void fbutil_destroy_attachments(Framebuffer *fb);
void fbutil_resize_attachment(Framebuffer *fb, FramebufferAttachment attachment, uint width, uint height);
