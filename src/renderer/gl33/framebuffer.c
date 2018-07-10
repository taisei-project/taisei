/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "framebuffer.h"
#include "core.h"

static GLuint r_attachment_to_gl_attachment[] = {
	[FRAMEBUFFER_ATTACH_DEPTH] = GL_DEPTH_ATTACHMENT,
	[FRAMEBUFFER_ATTACH_COLOR0] = GL_COLOR_ATTACHMENT0,
	[FRAMEBUFFER_ATTACH_COLOR1] = GL_COLOR_ATTACHMENT1,
	[FRAMEBUFFER_ATTACH_COLOR2] = GL_COLOR_ATTACHMENT2,
	[FRAMEBUFFER_ATTACH_COLOR3] = GL_COLOR_ATTACHMENT3,
};

static_assert(sizeof(r_attachment_to_gl_attachment)/sizeof(GLuint) == FRAMEBUFFER_MAX_ATTACHMENTS, "");

void gl33_framebuffer_create(Framebuffer *framebuffer) {
	memset(framebuffer, 0, sizeof(Framebuffer));
	framebuffer->impl = calloc(1, sizeof(FramebufferImpl));
	glGenFramebuffers(1, &framebuffer->impl->gl_fbo);
}

void gl33_framebuffer_attach(Framebuffer *framebuffer, Texture *tex, uint mipmap, FramebufferAttachment attachment) {
	assert(attachment >= 0 && attachment < FRAMEBUFFER_MAX_ATTACHMENTS);
	assert(mipmap < tex->impl->params.mipmaps);

	GLuint gl_tex = tex ? tex->impl->gl_handle : 0;
	Framebuffer *prev_fb = r_framebuffer_current();

	// make sure gl33_sync_framebuffer doesn't call gl33_framebuffer_initialize here
	framebuffer->impl->initialized = true;

	r_framebuffer(framebuffer);
	gl33_sync_framebuffer();
	glFramebufferTexture2D(GL_FRAMEBUFFER, r_attachment_to_gl_attachment[attachment], GL_TEXTURE_2D, gl_tex, mipmap);
	r_framebuffer(prev_fb);

	framebuffer->impl->attachments[attachment] = tex;
	framebuffer->impl->attachment_mipmaps[attachment] = mipmap;

	// need to update draw buffers
	framebuffer->impl->initialized = false;
}

Texture* gl33_framebuffer_get_attachment(Framebuffer *framebuffer, FramebufferAttachment attachment) {
	assert(framebuffer->impl != NULL);
	assert(attachment >= 0 && attachment < FRAMEBUFFER_MAX_ATTACHMENTS);

	if(!framebuffer->impl->initialized) {
		Framebuffer *prev_fb = r_framebuffer_current();
		r_framebuffer(framebuffer);
		gl33_sync_framebuffer(); // implicitly initializes!
		r_framebuffer(prev_fb);
	}

	return framebuffer->impl->attachments[attachment];
}


uint gl33_framebuffer_get_attachment_mipmap(Framebuffer *framebuffer, FramebufferAttachment attachment) {
	assert(framebuffer->impl != NULL);
	assert(attachment >= 0 && attachment < FRAMEBUFFER_MAX_ATTACHMENTS);

	return framebuffer->impl->attachment_mipmaps[attachment];
}

void gl33_framebuffer_destroy(Framebuffer *framebuffer) {
	if(framebuffer->impl != NULL) {
		gl33_framebuffer_deleted(framebuffer);
		glDeleteFramebuffers(1, &framebuffer->impl->gl_fbo);
		free(framebuffer->impl);
		framebuffer->impl = NULL;
	}
}

void gl33_framebuffer_taint(Framebuffer *framebuffer) {
	for(uint i = 0; i < FRAMEBUFFER_MAX_ATTACHMENTS; ++i) {
		if(framebuffer->impl->attachments[i] != NULL) {
			gl33_texture_taint(framebuffer->impl->attachments[i]);
		}
	}
}

void gl33_framebuffer_prepare(Framebuffer *framebuffer) {
	if(!framebuffer->impl->initialized) {
		// NOTE: this framebuffer is guaranteed to be active at this point

		if(glext.draw_buffers) {
			GLenum drawbufs[FRAMEBUFFER_MAX_COLOR_ATTACHMENTS];

			for(int i = 0; i < FRAMEBUFFER_MAX_COLOR_ATTACHMENTS; ++i) {
				if(framebuffer->impl->attachments[FRAMEBUFFER_ATTACH_COLOR0 + i] != NULL) {
					drawbufs[i] = GL_COLOR_ATTACHMENT0 + i;
				} else {
					drawbufs[i] = GL_NONE;
				}
			}

			glDrawBuffers(FRAMEBUFFER_MAX_COLOR_ATTACHMENTS, drawbufs);
		}
	}
}
