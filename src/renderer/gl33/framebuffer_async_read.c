/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
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
		rq->callback(&(Pixmap) {
			.width = rq->transfer.width,
			.height = rq->transfer.height,
			.format = rq->transfer.format,
			.origin = PIXMAP_ORIGIN_BOTTOMLEFT,
			.data_size = data_size,
			.data.untyped = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, data_size, GL_MAP_READ_BIT)
		}, rq->userdata);

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
}

void gl33_framebuffer_read_async(
	Framebuffer *framebuffer,
	FramebufferAttachment attachment,
	IntRect region,
	void *userdata,
	FramebufferReadAsyncCallback callback
) {
	GLTextureFormatInfo *fmtinfo = gl33_framebuffer_get_format(framebuffer, attachment);

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

	gl33_framebuffer_bind_for_read(framebuffer, attachment);
	glReadPixels(
		region.x, region.y, region.w, region.h,
		fmtinfo->transfer_format.gl_format, fmtinfo->transfer_format.gl_type,
		NULL
	);

	rq->sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	rq->sync_flags = GL_SYNC_FLUSH_COMMANDS_BIT;

	gl33_bind_buffer(GL33_BUFFER_BINDING_PIXEL_PACK, prev_pbo);
}
