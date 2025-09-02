/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "fbutil.h"

static const char* attachment_name(FramebufferAttachment a) {
	static const char *map[FRAMEBUFFER_MAX_ATTACHMENTS] = {
		[FRAMEBUFFER_ATTACH_DEPTH] = "depth",
		[FRAMEBUFFER_ATTACH_COLOR0] = "color0",
		[FRAMEBUFFER_ATTACH_COLOR1] = "color1",
		[FRAMEBUFFER_ATTACH_COLOR2] = "color2",
		[FRAMEBUFFER_ATTACH_COLOR3] = "color3",
	};

	assert((uint)a < FRAMEBUFFER_MAX_ATTACHMENTS);
	return map[(uint)a];
}

void fbutil_create_attachments(Framebuffer *fb, uint num_attachments, FBAttachmentConfig attachments[num_attachments]) {
	char buf[128];

	for(uint i = 0; i < num_attachments; ++i) {
		log_debug("%i %i", attachments[i].tex_params.width, attachments[i].tex_params.height);
		Texture *tex = r_texture_create(&attachments[i].tex_params);
		snprintf(buf, sizeof(buf), "%s [%s]", r_framebuffer_get_debug_label(fb), attachment_name(attachments[i].attachment));
		r_texture_set_debug_label(tex, buf);
		r_framebuffer_attach(fb, tex, 0, attachments[i].attachment);
	}
}

void fbutil_destroy_attachments(Framebuffer *fb) {
	for(uint i = 0; i < FRAMEBUFFER_MAX_ATTACHMENTS; ++i) {
		Texture *tex = r_framebuffer_get_attachment(fb, i);

		if(tex != NULL) {
			r_texture_destroy(tex);
		}
	}
}

void fbutil_resize_attachment(Framebuffer *fb, FramebufferAttachment attachment, uint width, uint height) {
	Texture *tex = r_framebuffer_get_attachment(fb, attachment);

	if(tex == NULL) {
		return;
	}

	uint tw, th;
	r_texture_get_size(tex, 0, &tw, &th);

	if(tw == width && th == height) {
		return;
	}

	// TODO: We could render a rescaled version of the old texture contents here

	TextureParams params;
	r_texture_get_params(tex, &params);
	r_texture_destroy(tex);
	params.width = width;
	params.height = height;
	params.mipmaps = 0; // FIXME
	r_framebuffer_attach(fb, r_texture_create(&params), 0, attachment);
}
