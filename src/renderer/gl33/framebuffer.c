/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "framebuffer.h"
#include "gl33.h"
#include "../glcommon/debug.h"

static GLuint r_attachment_to_gl_attachment[] = {
	[FRAMEBUFFER_ATTACH_DEPTH] = GL_DEPTH_ATTACHMENT,
	[FRAMEBUFFER_ATTACH_COLOR0] = GL_COLOR_ATTACHMENT0,
	[FRAMEBUFFER_ATTACH_COLOR1] = GL_COLOR_ATTACHMENT1,
	[FRAMEBUFFER_ATTACH_COLOR2] = GL_COLOR_ATTACHMENT2,
	[FRAMEBUFFER_ATTACH_COLOR3] = GL_COLOR_ATTACHMENT3,
};

static_assert(sizeof(r_attachment_to_gl_attachment)/sizeof(GLuint) == FRAMEBUFFER_MAX_ATTACHMENTS, "");

Framebuffer* gl33_framebuffer_create(void) {
	Framebuffer *fb = calloc(1, sizeof(Framebuffer));
	glGenFramebuffers(1, &fb->gl_fbo);
	snprintf(fb->debug_label, sizeof(fb->debug_label), "FBO #%i", fb->gl_fbo);
	return fb;
}

attr_unused static inline char* attachment_str(FramebufferAttachment a) {
	#define A(x) case x: return #x;
	switch(a) {
		A(FRAMEBUFFER_ATTACH_COLOR0)
		A(FRAMEBUFFER_ATTACH_COLOR1)
		A(FRAMEBUFFER_ATTACH_COLOR2)
		A(FRAMEBUFFER_ATTACH_COLOR3)
		A(FRAMEBUFFER_ATTACH_DEPTH)
		default: UNREACHABLE;
	}
	#undef A
}

void gl33_framebuffer_attach(Framebuffer *framebuffer, Texture *tex, uint mipmap, FramebufferAttachment attachment) {
	assert(attachment >= 0 && attachment < FRAMEBUFFER_MAX_ATTACHMENTS);
	assert(!tex || mipmap < tex->params.mipmaps);

	GLuint gl_tex = tex ? tex->gl_handle : 0;
	Framebuffer *prev_fb = r_framebuffer_current();

	// make sure gl33_sync_framebuffer doesn't call gl33_framebuffer_initialize here
	framebuffer->initialized = true;

	r_framebuffer(framebuffer);
	gl33_sync_framebuffer();
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, r_attachment_to_gl_attachment[attachment], GL_TEXTURE_2D, gl_tex, mipmap);
	r_framebuffer(prev_fb);

	framebuffer->attachments[attachment] = tex;
	framebuffer->attachment_mipmaps[attachment] = mipmap;

	// need to update draw buffers
	framebuffer->initialized = false;

	if(tex) {
		log_debug("%s %s = %s (%ux%u mip %u)", framebuffer->debug_label, attachment_str(attachment), tex->debug_label, tex->params.width, tex->params.height, mipmap);
	} else {
		log_debug("%s %s = NULL", framebuffer->debug_label, attachment_str(attachment));
	}
}

Texture* gl33_framebuffer_get_attachment(Framebuffer *framebuffer, FramebufferAttachment attachment) {
	assert(attachment >= 0 && attachment < FRAMEBUFFER_MAX_ATTACHMENTS);
	return framebuffer->attachments[attachment];
}

uint gl33_framebuffer_get_attachment_mipmap(Framebuffer *framebuffer, FramebufferAttachment attachment) {
	assert(attachment >= 0 && attachment < FRAMEBUFFER_MAX_ATTACHMENTS);
	return framebuffer->attachment_mipmaps[attachment];
}

void gl33_framebuffer_destroy(Framebuffer *framebuffer) {
	gl33_framebuffer_deleted(framebuffer);
	glDeleteFramebuffers(1, &framebuffer->gl_fbo);
	free(framebuffer);
}

void gl33_framebuffer_taint(Framebuffer *framebuffer) {
	for(uint i = 0; i < FRAMEBUFFER_MAX_ATTACHMENTS; ++i) {
		if(framebuffer->attachments[i] != NULL) {
			gl33_texture_taint(framebuffer->attachments[i]);
		}
	}
}

void gl33_framebuffer_prepare(Framebuffer *framebuffer) {
	if(!framebuffer->initialized) {
		// NOTE: this framebuffer is guaranteed to be bound at this point

		if(glext.draw_buffers) {
			GLenum drawbufs[FRAMEBUFFER_MAX_COLOR_ATTACHMENTS];

			for(int i = 0; i < FRAMEBUFFER_MAX_COLOR_ATTACHMENTS; ++i) {
				if(framebuffer->attachments[FRAMEBUFFER_ATTACH_COLOR0 + i] != NULL) {
					drawbufs[i] = GL_COLOR_ATTACHMENT0 + i;
				} else {
					drawbufs[i] = GL_NONE;
				}
			}

			glDrawBuffers(FRAMEBUFFER_MAX_COLOR_ATTACHMENTS, drawbufs);
		}

		framebuffer->initialized = true;
	}
}

void gl33_framebuffer_set_debug_label(Framebuffer *fb, const char *label) {
	glcommon_set_debug_label(fb->debug_label, "FBO", GL_FRAMEBUFFER, fb->gl_fbo, label);
}

const char* gl33_framebuffer_get_debug_label(Framebuffer* fb) {
	return fb->debug_label;
}

void gl33_framebuffer_clear(Framebuffer *framebuffer, ClearBufferFlags flags, const Color *colorval, float depthval) {
	GLuint glflags = 0;

	if(flags & CLEAR_COLOR) {
		glflags |= GL_COLOR_BUFFER_BIT;
		assert(colorval != NULL);
		gl33_set_clear_color(colorval);
	}

	if(flags & CLEAR_DEPTH) {
		glflags |= GL_DEPTH_BUFFER_BIT;
		gl33_set_clear_depth(depthval);
	}

	r_flush_sprites();

	Framebuffer *fb_saved = r_framebuffer_current();
	r_framebuffer(framebuffer);
	gl33_sync_framebuffer();
	glClear(glflags);
	r_framebuffer(fb_saved);
}
