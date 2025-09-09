/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "fbpair.h"

static void fbpair_destroy_fb(Framebuffer *fb) {
	fbutil_destroy_attachments(fb);
	r_framebuffer_destroy(fb);
}

static void fbpair_resize_fb(Framebuffer *fb, FramebufferAttachment attachment, uint width, uint height) {
	fbutil_resize_attachment(fb, attachment, width, height);
}

void fbpair_create(FBPair *pair, uint num_attachments, FBAttachmentConfig attachments[num_attachments], const char *debug_label) {
	assert(num_attachments > 0 && num_attachments <= FRAMEBUFFER_MAX_ATTACHMENTS);
	*pair = (typeof(*pair)) {};

	char buf[R_DEBUG_LABEL_SIZE];

	snprintf(buf, sizeof(buf), "%s FB 1", debug_label);
	pair->front = r_framebuffer_create();
	r_framebuffer_set_debug_label(pair->front, buf);
	fbutil_create_attachments(pair->front, num_attachments, attachments);

	snprintf(buf, sizeof(buf), "%s FB 2", debug_label);
	pair->back = r_framebuffer_create();
	r_framebuffer_set_debug_label(pair->back, buf);
	fbutil_create_attachments(pair->back, num_attachments, attachments);
}

void fbpair_destroy(FBPair *pair) {
	fbpair_destroy_fb(pair->front);
	fbpair_destroy_fb(pair->back);
}

void fbpair_swap(FBPair *pair) {
	void *tmp = pair->front;
	pair->front = pair->back;
	pair->back  = tmp;
}

static void fbpair_clear(FBPair *pair) {
	r_framebuffer_clear(pair->front, BUFFER_ALL, RGBA(0, 0, 0, 0), 1);
	r_framebuffer_clear(pair->back, BUFFER_ALL, RGBA(0, 0, 0, 0), 1);
}

void fbpair_resize(FBPair *pair, FramebufferAttachment attachment, uint width, uint height) {
	fbpair_resize_fb(pair->front, attachment, width, height);
	fbpair_resize_fb(pair->back, attachment, width, height);
	fbpair_clear(pair);
}

void fbpair_resize_all(FBPair *pair, uint width, uint height) {
	for(uint i = 0; i < FRAMEBUFFER_MAX_ATTACHMENTS; ++i) {
		fbpair_resize_fb(pair->front, i, width, height);
		fbpair_resize_fb(pair->back, i, width, height);
	}

	fbpair_clear(pair);
}

void fbpair_viewport(FBPair *pair, float x, float y, float w, float h) {
	r_framebuffer_viewport(pair->front, x, y, w, h);
	r_framebuffer_viewport(pair->back, x, y, w, h);
}

void fbpair_viewport_rect(FBPair *pair, FloatRect vp) {
	r_framebuffer_viewport_rect(pair->front, vp);
	r_framebuffer_viewport_rect(pair->back, vp);
}
