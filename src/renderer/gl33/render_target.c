/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "render_target.h"
#include "core.h"

static GLuint r_attachment_to_gl_attachment[] = {
	[RENDERTARGET_ATTACHMENT_DEPTH] = GL_DEPTH_ATTACHMENT,
	[RENDERTARGET_ATTACHMENT_COLOR0] = GL_COLOR_ATTACHMENT0,
	[RENDERTARGET_ATTACHMENT_COLOR1] = GL_COLOR_ATTACHMENT1,
	[RENDERTARGET_ATTACHMENT_COLOR2] = GL_COLOR_ATTACHMENT2,
	[RENDERTARGET_ATTACHMENT_COLOR3] = GL_COLOR_ATTACHMENT3,
};

static_assert(sizeof(r_attachment_to_gl_attachment)/sizeof(GLuint) == RENDERTARGET_MAX_ATTACHMENTS, "");

void r_target_create(RenderTarget *target) {
	assert(target != NULL);
	memset(target, 0, sizeof(RenderTarget));
	target->impl = calloc(1, sizeof(RenderTargetImpl));
	glGenFramebuffers(1, &target->impl->gl_fbo);
}

void r_target_attach(RenderTarget *target, Texture *tex, RenderTargetAttachment attachment) {
	assert(attachment >= 0 && attachment < RENDERTARGET_MAX_ATTACHMENTS);
	assert(tex != NULL);

	GLuint gl_tex = tex ? tex->impl->gl_handle : 0;
	RenderTarget *prev_target = r_target_current();

	// make sure gl33_sync_render_target doesn't call gl33_target_initialize here
	target->impl->initialized = true;

	r_target(target);
	gl33_sync_render_target();
	glFramebufferTexture2D(GL_FRAMEBUFFER, r_attachment_to_gl_attachment[attachment], GL_TEXTURE_2D, gl_tex, 0);
	r_target(prev_target);

	target->impl->attachments[attachment] = tex;

	// need to update draw buffers
	target->impl->initialized = false;
}

Texture* r_target_get_attachment(RenderTarget *target, RenderTargetAttachment attachment) {
	assert(target->impl != NULL);
	assert(attachment >= 0 && attachment < RENDERTARGET_MAX_ATTACHMENTS);

	if(!target->impl->initialized) {
		RenderTarget *prev_target = r_target_current();
		r_target(target);
		gl33_sync_render_target(); // implicitly initializes!
		r_target(prev_target);
	}

	return target->impl->attachments[attachment];
}

void r_target_destroy(RenderTarget *target) {
	assert(target != NULL);
	glDeleteFramebuffers(1, &target->impl->gl_fbo);
	free(target->impl);
}

void gl33_target_initialize(RenderTarget *target) {
	if(!target->impl->initialized) {
		// NOTE: this render target is guaranteed to be active at this point

		GLenum drawbufs[RENDERTARGET_MAX_COLOR_ATTACHMENTS];

		for(int i = 0; i < RENDERTARGET_MAX_COLOR_ATTACHMENTS; ++i) {
			if(target->impl->attachments[RENDERTARGET_ATTACHMENT_COLOR0 + i] != NULL) {
				drawbufs[i] = GL_COLOR_ATTACHMENT0 + i;
			} else {
				drawbufs[i] = GL_NONE;
			}
		}

		glDrawBuffers(RENDERTARGET_MAX_COLOR_ATTACHMENTS, drawbufs);

		// NOTE: r_clear would cause an infinite recursion here
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		target->impl->initialized = true;
	}
}
