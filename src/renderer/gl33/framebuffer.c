/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
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

Framebuffer *gl33_framebuffer_create(void) {
	auto fb = ALLOC(Framebuffer);
	glGenFramebuffers(1, &fb->gl_fbo);
	snprintf(fb->debug_label, sizeof(fb->debug_label), "FBO #%i", fb->gl_fbo);

	for(int i = 0; i < FRAMEBUFFER_MAX_OUTPUTS; ++i) {
		fb->output_mapping[i] = FRAMEBUFFER_ATTACH_COLOR0 + i;
	}

	// According to the GL spec, the FBO is only actually created when it's first bound.
	// Let's make sure this happens as early as possible.
	Framebuffer *prev_fb = r_framebuffer_current();
	r_framebuffer(fb);
	gl33_sync_framebuffer();
	r_framebuffer(prev_fb);

	return fb;
}

attr_unused static inline char *attachment_str(FramebufferAttachment a) {
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
	assert(tex || mipmap == 0);

	GLuint gl_tex = tex ? tex->gl_handle : 0;
	Framebuffer *prev_fb = r_framebuffer_current();

	// gl33_sync_framebuffer will call gl33_framebuffer_prepare; we don't want it to touch draw buffers yet.
	framebuffer->draw_buffers_dirty = false;

	r_framebuffer(framebuffer);
	gl33_sync_framebuffer();
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, r_attachment_to_gl_attachment[attachment], GL_TEXTURE_2D, gl_tex, mipmap);
	r_framebuffer(prev_fb);

	framebuffer->attachments[attachment] = tex;
	framebuffer->attachment_mipmaps[attachment] = mipmap;

	// need to update draw buffers
	framebuffer->draw_buffers_dirty = true;

	IF_DEBUG(if(tex) {
		log_debug("%s %s = %s (%ux%u mip %u)", framebuffer->debug_label, attachment_str(attachment), tex->debug_label, tex->params.width, tex->params.height, mipmap);
	} else {
		log_debug("%s %s = NULL", framebuffer->debug_label, attachment_str(attachment));
	});
}

FramebufferAttachmentQueryResult gl33_framebuffer_query_attachment(Framebuffer *framebuffer, FramebufferAttachment attachment) {
	assert(attachment >= 0 && attachment < FRAMEBUFFER_MAX_ATTACHMENTS);
	return (FramebufferAttachmentQueryResult) {
		.texture = framebuffer->attachments[attachment],
		.miplevel = framebuffer->attachment_mipmaps[attachment],
	};
}

void gl33_framebuffer_outputs(
	Framebuffer *framebuffer,
	FramebufferAttachment config[FRAMEBUFFER_MAX_OUTPUTS],
	uint8_t write_mask
) {
	if(write_mask == 0x00) {
		memcpy(config, framebuffer->output_mapping, sizeof(framebuffer->output_mapping));
		return;
	}

	for(int i = 0; i < FRAMEBUFFER_MAX_OUTPUTS; ++i) {
		if(write_mask & (1 << i) && config[i] != framebuffer->output_mapping[i]) {
			if(config[i] != FRAMEBUFFER_ATTACH_NONE) {
				assert(config[i] >= FRAMEBUFFER_ATTACH_COLOR0);
				assert(config[i] < FRAMEBUFFER_ATTACH_COLOR0 + FRAMEBUFFER_MAX_COLOR_ATTACHMENTS);
			}

			framebuffer->output_mapping[i] = config[i];
			framebuffer->draw_buffers_dirty = true;
		}
	}
}

void gl33_framebuffer_destroy(Framebuffer *framebuffer) {
	gl33_framebuffer_deleted(framebuffer);
	glDeleteFramebuffers(1, &framebuffer->gl_fbo);
	mem_free(framebuffer);
}

void gl33_framebuffer_taint(Framebuffer *framebuffer) {
	for(uint i = 0; i < FRAMEBUFFER_MAX_ATTACHMENTS; ++i) {
		if(framebuffer->attachments[i] != NULL) {
			gl33_texture_taint(framebuffer->attachments[i]);
		}
	}
}

void gl33_framebuffer_prepare(Framebuffer *framebuffer) {
	if(framebuffer->draw_buffers_dirty) {
		// NOTE: this framebuffer is guaranteed to be bound at this point

		if(glext.draw_buffers) {
			GLenum drawbufs[FRAMEBUFFER_MAX_OUTPUTS];

			for(int i = 0; i < FRAMEBUFFER_MAX_OUTPUTS; ++i) {
				FramebufferAttachment a = framebuffer->output_mapping[i];

				if(a == FRAMEBUFFER_ATTACH_NONE || framebuffer->attachments[a] == NULL) {
					drawbufs[i] = GL_NONE;
				} else {
					drawbufs[i] = GL_COLOR_ATTACHMENT0 + (a - FRAMEBUFFER_ATTACH_COLOR0);
				}
			}

			glDrawBuffers(FRAMEBUFFER_MAX_OUTPUTS, drawbufs);
		} else {
			// TODO: maybe simulate this by modifying framebuffer attachments?
		}

		framebuffer->draw_buffers_dirty = false;
	}
}

void gl33_framebuffer_set_debug_label(Framebuffer *fb, const char *label) {
	glcommon_set_debug_label(fb->debug_label, "FBO", GL_FRAMEBUFFER, fb->gl_fbo, label);
}

const char *gl33_framebuffer_get_debug_label(Framebuffer* fb) {
	return fb->debug_label;
}

static GLuint buffer_flags_to_gl(BufferKindFlags flags) {
	GLuint glflags = 0;

	if(flags & BUFFER_COLOR) {
		glflags |= GL_COLOR_BUFFER_BIT;
	}

	if(flags & BUFFER_DEPTH) {
		glflags |= GL_DEPTH_BUFFER_BIT;
	}

	return glflags;
}

void gl33_framebuffer_clear(Framebuffer *framebuffer, BufferKindFlags flags, const Color *colorval, float depthval) {
	GLuint glflags = buffer_flags_to_gl(flags);

	if(flags & BUFFER_COLOR) {
		assert(colorval != NULL);
		gl33_set_clear_color(colorval);
	}

	if(flags & BUFFER_DEPTH) {
		gl33_set_clear_depth(depthval);
	}

	r_flush_sprites();

	Framebuffer *fb_saved = r_framebuffer_current();
	r_framebuffer(framebuffer);
	gl33_sync_framebuffer();
	gl33_sync_scissor();
	glClear(glflags);
	r_framebuffer(fb_saved);
}

void gl33_framebuffer_copy(Framebuffer *dst, Framebuffer *src, BufferKindFlags flags) {
	GLuint glflags = buffer_flags_to_gl(flags);

	r_flush_sprites();

	IntExtent size = r_framebuffer_get_size(dst);
	GLint X0 = 0;
	GLint X1 = size.w;
	GLint Y0 = 0;
	GLint Y1 = size.h;

	Framebuffer *fb_saved = r_framebuffer_current();
	r_framebuffer(dst);
	gl33_sync_framebuffer();
	gl33_sync_scissor();
	// TODO track this?
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->gl_fbo);
	glBlitFramebuffer(X0, Y0, X1, Y1, X0, Y0, X1, Y1, glflags, GL_NEAREST);
	r_framebuffer(fb_saved);
}

IntExtent gl33_framebuffer_get_effective_size(Framebuffer *framebuffer) {
	// According to the OpenGL wiki:
	// "The effective size of the FBO is the intersection of all of the sizes of the bound images (ie: the smallest in each dimension)."

	// TODO: do it once in gl33_framebuffer_prepare and cache the result

	IntExtent fb_size = { 0, 0 };

	for(int i = 0; i < FRAMEBUFFER_MAX_ATTACHMENTS; ++i) {
		Texture *tex = framebuffer->attachments[i];

		if(tex != NULL) {
			uint mipmap = framebuffer->attachment_mipmaps[i];
			uint tex_w, tex_h;
			gl33_texture_get_size(tex, mipmap, &tex_w, &tex_h);
			IntExtent tex_size = { tex_w, tex_h };

			if(fb_size.w == 0 && fb_size.h == 0) {
				fb_size = tex_size;
			} else if(tex_size.w < fb_size.w) {
				fb_size.w = tex_size.w;
			} else if(tex_size.h < fb_size.h) {
				fb_size.h = tex_size.h;
			}
		}
	}

	return fb_size;
}

typedef struct FramebufferReadRequest {
	Pixmap *target;
	Allocator *allocator;
	void *userdata;
	FramebufferReadAsyncCallback callback;
	GLuint pbo;
	GLsync sync;
	GLenum sync_result;
	GLbitfield sync_flags;
} FramebufferReadRequest;

static FramebufferReadRequest read_requests[4];

static void handle_read_request(FramebufferReadRequest *rq, bool ok) {
	glDeleteSync(NOT_NULL(rq->sync));
	rq->sync = NULL;
	rq->sync_result = GL_NONE;

	auto pxm = rq->target;

	if(ok) {
		// data_size already specified
		pxm->data.untyped = allocator_alloc(rq->allocator, pxm->data_size);
		assert(rq->pbo != 0);
		auto prev_pbo = gl33_buffer_current(GL33_BUFFER_BINDING_PIXEL_PACK);
		gl33_bind_buffer(GL33_BUFFER_BINDING_PIXEL_PACK, rq->pbo);
		gl33_sync_buffer(GL33_BUFFER_BINDING_PIXEL_PACK);
		glGetBufferSubData(GL_PIXEL_PACK_BUFFER, 0, pxm->data_size, pxm->data.untyped);
		gl33_bind_buffer(GL33_BUFFER_BINDING_PIXEL_PACK, prev_pbo);
	} else {
		pxm->data_size = 0;
		pxm->data.untyped = NULL;
	}

	rq->callback(rq->allocator, rq->target, rq->userdata);
}

static GLenum sync_read_request(FramebufferReadRequest *rq, GLuint64 timeout) {
	if(!rq->sync) {
		return GL_NONE;
	}

	rq->sync_result = glClientWaitSync(rq->sync, rq->sync_flags, timeout);
	rq->sync_flags = 0;

	switch(rq->sync_result) {
		case GL_ALREADY_SIGNALED:
		case GL_CONDITION_SATISFIED:
			handle_read_request(rq, true);
			break;
		case GL_WAIT_FAILED:
			handle_read_request(rq, false);
			break;
		case GL_TIMEOUT_EXPIRED:
			break;
		default: UNREACHABLE;
	}

	return rq->sync_result;
}

static FramebufferReadRequest *alloc_read_request_timeout(GLuint64 timeout) {
	for(int i = 0; i < ARRAY_SIZE(read_requests); ++i) {
		auto rq = read_requests + i;
		if(sync_read_request(rq, timeout) == GL_NONE) {
			return rq;
		}
	}

	return NULL;
}

static FramebufferReadRequest *alloc_read_request(void) {
	auto rq = alloc_read_request_timeout(0);

	if(rq) {
		return rq;
	}

	log_warn("Queue is full, forcing synchronization");
	rq = alloc_read_request_timeout(UINT64_MAX);
	return NOT_NULL(rq);
}

void gl33_framebuffer_process_read_requests(void) {
	for(int i = 0; i < ARRAY_SIZE(read_requests); ++i) {
		sync_read_request(read_requests + i, 0);
	}
}

void gl33_framebuffer_finalize_read_requests(void) {
	gl33_framebuffer_process_read_requests();

	for(int i = 0; i < ARRAY_SIZE(read_requests); ++i) {
		auto rq = read_requests + 1;
		sync_read_request(rq, UINT64_MAX);

		if(rq->sync) {
			glDeleteSync(rq->sync);
			rq->sync = NULL;
			rq->sync_result = GL_NONE;
		}

		if(rq->pbo) {
			glDeleteBuffers(1, &rq->pbo);
			rq->pbo = 0;
		}
	}
}

void gl33_framebuffer_read_async(
	Framebuffer *framebuffer,
	FramebufferAttachment attachment,
	IntRect region,
	Allocator *allocator,
	Pixmap *pxm,
	void *userdata,
	FramebufferReadAsyncCallback callback
) {
	GLTextureFormatInfo *fmtinfo = NULL;

	if(framebuffer) {
		assert(attachment >= 0 && attachment < FRAMEBUFFER_MAX_ATTACHMENTS);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer->gl_fbo);
		glReadBuffer(r_attachment_to_gl_attachment[attachment]);
		FramebufferAttachmentQueryResult aq = gl33_framebuffer_query_attachment(framebuffer, attachment);
		fmtinfo = NOT_NULL(aq.texture)->fmt_info;
	} else {
		assert(attachment == FRAMEBUFFER_ATTACH_NONE);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glReadBuffer(GL_BACK);

		GLenum ifmt = GL_RGBA8;  // FIXME how to query this?

		auto formats = glcommon_get_texture_formats();
		dynarray_foreach_elem(formats, auto fmt, {
			if(fmt->internal_format == ifmt) {
				fmtinfo = fmt;
				break;
			}
		});

		assume(fmtinfo != NULL);
	}

	*pxm = (Pixmap) {
		.width = region.w,
		.height = region.h,
		.format = fmtinfo->transfer_format.pixmap_format,
		.origin = PIXMAP_ORIGIN_BOTTOMLEFT,
	};

	auto rq = alloc_read_request();
	rq->allocator = allocator;
	rq->target = pxm;
	rq->userdata = userdata;
	rq->callback = callback;
	rq->sync_flags = GL_SYNC_FLUSH_COMMANDS_BIT;

	if(!rq->pbo) {
		glGenBuffers(1, &rq->pbo);
	}

	pxm->data_size = pixmap_data_size(pxm->format, pxm->width, pxm->height);

	auto prev_pbo = gl33_buffer_current(GL33_BUFFER_BINDING_PIXEL_PACK);
	gl33_bind_buffer(GL33_BUFFER_BINDING_PIXEL_PACK, rq->pbo);
	gl33_sync_buffer(GL33_BUFFER_BINDING_PIXEL_PACK);
	glBufferData(GL_PIXEL_PACK_BUFFER, pxm->data_size, NULL, GL_STREAM_READ);

	glReadPixels(
		region.x, region.y, region.w, region.h,
		fmtinfo->transfer_format.gl_format, fmtinfo->transfer_format.gl_type,
		NULL
	);

	rq->sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	rq->sync_flags = GL_SYNC_FLUSH_COMMANDS_BIT;

	gl33_bind_buffer(GL33_BUFFER_BINDING_PIXEL_PACK, prev_pbo);
}
