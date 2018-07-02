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

static void fbpair_create_fb(Framebuffer *fb, uint num_attachments, FBAttachmentConfig attachments[num_attachments]) {
	r_framebuffer_create(fb);

	for(uint i = 0; i < num_attachments; ++i) {
		Texture *tex = calloc(1, sizeof(Texture));
		r_texture_create(tex, &attachments[i].tex_params);
		r_framebuffer_attach(fb, tex, attachments[i].attachment);
	}
}

static void fbpair_destroy_fb(Framebuffer *fb) {
	for(uint i = 0; i < FRAMEBUFFER_MAX_ATTACHMENTS; ++i) {
		Texture *tex = r_framebuffer_get_attachment(fb, i);

		if(tex != NULL) {
			r_texture_destroy(tex);
			free(tex);
		}
	}

	r_framebuffer_destroy(fb);
}

static void fbpair_resize_fb(Framebuffer *fb, FramebufferAttachment attachment, uint width, uint height) {
	Texture *tex = r_framebuffer_get_attachment(fb, attachment);

	if(tex == NULL || (tex->w == width && tex->h == height)) {
		return;
	}

	// TODO: We could render a rescaled version of the old texture contents here

	r_texture_replace(tex, tex->type, width, height, NULL);
	r_state_push();
	r_framebuffer(fb);
	r_clear_color4(0, 0, 0, 0);
	r_clear(CLEAR_ALL);
	r_state_pop();
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

void fbpair_resize(FBPair *pair, FramebufferAttachment attachment, uint width, uint height) {
	fbpair_resize_fb(pair->framebuffers + 0, attachment, width, height);
	fbpair_resize_fb(pair->framebuffers + 1, attachment, width, height);
}

void fbpair_resize_all(FBPair *pair, uint width, uint height) {
	for(uint i = 0; i < FRAMEBUFFER_MAX_ATTACHMENTS; ++i) {
		fbpair_resize(pair, i, width, height);
	}
}
