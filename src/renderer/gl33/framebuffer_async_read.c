/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "framebuffer_async_read.h"
#include "framebuffer.h"

#include "../glcommon/texture.h"
#include "gl33.h"
#include "opengl.h"
#include "util.h"

typedef struct FramebufferReadRequest {
	FramebufferReadAsyncCallback callback;
	void *userdata;
	GLuint pbo;
	GLsync sync;
	GLenum sync_result;
	GLbitfield sync_flags;
	struct {
		uint width;
		uint height;
		PixmapFormat format;
	} transfer;
} FramebufferReadRequest;

static FramebufferReadRequest read_requests[4];

// When reading from the default framebuffer, we will get a Y-flipped image.
// Set up a renderbuffer to flip it on the GPU before presenting it.
static struct {
	Framebuffer fb;
	GLuint rbo;
	uint width;
	uint height;
	GLenum internal_format;
} flip_fbo;

static void setup_flip_fbo(uint width, uint height, GLenum internal_format) {
	if(!flip_fbo.fb.gl_fbo) {
		glGenFramebuffers(1, &flip_fbo.fb.gl_fbo);
		glGenRenderbuffers(1, &flip_fbo.rbo);
		snprintf(flip_fbo.fb.debug_label, sizeof(flip_fbo.fb.debug_label), "FBO #%i", flip_fbo.fb.gl_fbo);
		gl33_framebuffer_set_debug_label(&flip_fbo.fb, "Async read flip buffer");
	}

	if(flip_fbo.width != width || flip_fbo.height != height || flip_fbo.internal_format != internal_format) {
		glBindRenderbuffer(GL_RENDERBUFFER, flip_fbo.rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, internal_format, width, height);

		Framebuffer *prev_fb = r_framebuffer_current();
		r_framebuffer(&flip_fbo.fb);
		gl33_sync_framebuffer();
		glBindFramebuffer(GL_FRAMEBUFFER, flip_fbo.fb.gl_fbo);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, flip_fbo.rbo);
		glDrawBuffers(1, (GLenum[]) { GL_COLOR_ATTACHMENT0 });
		r_framebuffer(prev_fb);

		flip_fbo.width = width;
		flip_fbo.height = height;
		flip_fbo.internal_format = internal_format;
	}
}

static void handle_read_request(FramebufferReadRequest *rq, bool ok) {
	glDeleteSync(NOT_NULL(rq->sync));
	rq->sync = NULL;
	rq->sync_result = GL_NONE;

	if(ok) {
		assert(rq->pbo != 0);
		auto prev_pbo = gl33_buffer_current(GL33_BUFFER_BINDING_PIXEL_PACK);
		gl33_bind_buffer(GL33_BUFFER_BINDING_PIXEL_PACK, rq->pbo);
		gl33_sync_buffer(GL33_BUFFER_BINDING_PIXEL_PACK);

		auto data_size = pixmap_data_size(rq->transfer.format, rq->transfer.width, rq->transfer.height);
		Pixmap px = {
			.width = rq->transfer.width,
			.height = rq->transfer.height,
			.format = rq->transfer.format,
			.data_size = data_size,
			.data.untyped = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, data_size, GL_MAP_READ_BIT)
		};

		rq->callback(&px, rq->userdata);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		gl33_bind_buffer(GL33_BUFFER_BINDING_PIXEL_PACK, prev_pbo);

		// TODO invalidate
	} else {
		rq->callback(NULL, rq->userdata);
	}
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
		case GL_NONE:
			log_warn("BUG: glClientWaitSync() returned GL_NONE?");
			glDeleteSync(rq->sync);
			rq->sync = NULL;
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
		auto rq = read_requests + i;
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

	if(flip_fbo.fb.gl_fbo) {
		glDeleteFramebuffers(1, &flip_fbo.fb.gl_fbo);
		glDeleteRenderbuffers(1, &flip_fbo.rbo);
		flip_fbo = (typeof(flip_fbo)) {};
	}
}

void gl33_framebuffer_read_async(
	Framebuffer *framebuffer,
	FramebufferAttachment attachment,
	IntRect region,
	void *userdata,
	FramebufferReadAsyncCallback callback
) {
	GLTextureFormatInfo *fmtinfo = gl33_framebuffer_get_format(framebuffer, attachment);
	bool flip = framebuffer == NULL;

	auto rq = alloc_read_request();
	rq->transfer.width = region.w;
	rq->transfer.height = region.h;
	rq->transfer.format = fmtinfo->transfer_format.pixmap_format;
	rq->userdata = userdata;
	rq->callback = callback;
	rq->sync_flags = GL_SYNC_FLUSH_COMMANDS_BIT;

	if(!rq->pbo) {
		glGenBuffers(1, &rq->pbo);
	}

	auto prev_pbo = gl33_buffer_current(GL33_BUFFER_BINDING_PIXEL_PACK);
	gl33_bind_buffer(GL33_BUFFER_BINDING_PIXEL_PACK, rq->pbo);
	gl33_sync_buffer(GL33_BUFFER_BINDING_PIXEL_PACK);
	auto data_size = pixmap_data_size(rq->transfer.format, rq->transfer.width, rq->transfer.height);
	glBufferData(GL_PIXEL_PACK_BUFFER, data_size, NULL, GL_STREAM_READ);

	if(flip) {
		setup_flip_fbo(region.w, region.h, fmtinfo->internal_format);

		Framebuffer *prev_fb = r_framebuffer_current();
		r_framebuffer(&flip_fbo.fb);
		gl33_sync_framebuffer();
		gl33_framebuffer_bind_for_read(NULL, FRAMEBUFFER_ATTACH_NONE);

		glBlitFramebuffer(
			region.x, region.y, region.x + region.w, region.y + region.h,
			0,        region.h,            region.w,            0,
			GL_COLOR_BUFFER_BIT, GL_NEAREST
		);

		gl33_framebuffer_bind_for_read(&flip_fbo.fb, FRAMEBUFFER_ATTACH_COLOR0);
		glReadPixels(
			0, 0, region.w, region.h,
			fmtinfo->transfer_format.gl_format, fmtinfo->transfer_format.gl_type,
			NULL
		);

		r_framebuffer(prev_fb);
	} else {
		gl33_framebuffer_bind_for_read(framebuffer, attachment);
		glReadPixels(
			region.x, region.y, region.w, region.h,
			fmtinfo->transfer_format.gl_format, fmtinfo->transfer_format.gl_type,
			NULL
		);
	}

	rq->sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	rq->sync_flags = GL_SYNC_FLUSH_COMMANDS_BIT;

	gl33_bind_buffer(GL33_BUFFER_BINDING_PIXEL_PACK, prev_pbo);
}
