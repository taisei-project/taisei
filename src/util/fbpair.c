/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "fbpair.h"
#include "global.h"
#include "util.h"
#include "util/graphics.h"

static void fbpair_create_fb(Framebuffer *fb, uint num_attachments, FBAttachmentConfig attachments[num_attachments]) {
	r_framebuffer_create(fb);
	fbutil_create_attachments(fb, num_attachments, attachments);
}

static void fbpair_destroy_fb(Framebuffer *fb) {
	fbutil_destroy_attachments(fb);
	r_framebuffer_destroy(fb);
}

static void fbpair_resize_fb(Framebuffer *fb, FramebufferAttachment attachment, uint width, uint height) {
	fbutil_resize_attachment(fb, attachment, width, height);
}

void fbpair_create(FBPair *pair, uint num_attachments, FBAttachmentConfig attachments[num_attachments]) {
	assert(num_attachments > 0 && num_attachments <= FRAMEBUFFER_MAX_ATTACHMENTS);
	memset(pair, 0, sizeof(*pair));
	fbpair_create_fb(*(void**)&pair->front = pair->framebuffers + 0, num_attachments, attachments);
	fbpair_create_fb(*(void**)&pair->back  = pair->framebuffers + 1, num_attachments, attachments);
}

void fbpair_destroy(FBPair *pair) {
	fbpair_destroy_fb(pair->framebuffers + 0);
	fbpair_destroy_fb(pair->framebuffers + 1);
}

void fbpair_swap(FBPair *pair) {
	void *tmp = pair->front;
	*(void**)&pair->front = pair->back;
	*(void**)&pair->back  = tmp;
}

static void fbpair_clear(FBPair *pair) {
	r_state_push();
	r_clear_color4(0, 0, 0, 0);
	r_framebuffer(pair->framebuffers + 0);
	r_clear(CLEAR_ALL);
	r_framebuffer(pair->framebuffers + 1);
	r_clear(CLEAR_ALL);
	r_state_pop();
}

void fbpair_resize(FBPair *pair, FramebufferAttachment attachment, uint width, uint height) {
	fbpair_resize_fb(pair->framebuffers + 0, attachment, width, height);
	fbpair_resize_fb(pair->framebuffers + 1, attachment, width, height);
	fbpair_clear(pair);
}

void fbpair_resize_all(FBPair *pair, uint width, uint height) {
	for(uint i = 0; i < FRAMEBUFFER_MAX_ATTACHMENTS; ++i) {
		fbpair_resize_fb(pair->framebuffers + 0, i, width, height);
		fbpair_resize_fb(pair->framebuffers + 1, i, width, height);
	}

	fbpair_clear(pair);
}

void fbpair_viewport(FBPair *pair, int x, int y, int w, int h) {
	r_framebuffer_viewport(pair->front, x, y, w, h);
	r_framebuffer_viewport(pair->back, x, y, w, h);
}

void fbpair_viewport_rect(FBPair *pair, IntRect vp) {
	r_framebuffer_viewport_rect(pair->front, vp);
	r_framebuffer_viewport_rect(pair->back, vp);
}
