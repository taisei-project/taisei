/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "framebuffer_async_read.h"
#include "framebuffer.h"

void gl33_framebuffer_read_async(
	Framebuffer *framebuffer,
	FramebufferAttachment attachment,
	IntRect region,
	void *userdata,
	FramebufferReadAsyncCallback callback
) {
	GLTextureFormatInfo *fmtinfo = gl33_framebuffer_get_format(framebuffer, attachment);

	Pixmap pxm = {
		.width = region.w,
		.height = region.h,
		.format = fmtinfo->transfer_format.pixmap_format,
		.origin = PIXMAP_ORIGIN_BOTTOMLEFT,
	};

	pxm.data.untyped = pixmap_alloc_buffer(pxm.format, pxm.width, pxm.height, &pxm.data_size);

	gl33_framebuffer_bind_for_read(framebuffer, attachment);
	glReadPixels(
		region.x, region.y, region.w, region.h,
		fmtinfo->transfer_format.gl_format, fmtinfo->transfer_format.gl_type,
		pxm.data.untyped);

	callback(&pxm, userdata);
	mem_free(pxm.data.untyped);
}

void gl33_framebuffer_process_read_requests(void) { }
void gl33_framebuffer_finalize_read_requests(void) { }
