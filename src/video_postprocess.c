/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "video_postprocess.h"

#include "resource/postprocess.h"
#include "util/fbmgr.h"
#include "util/graphics.h"
#include "video.h"

struct VideoPostProcess {
	ManagedFramebufferGroup *mfb_group;
	FBPair framebuffers;
	PostprocessShader *pp_pipeline;
	int frames;
};

VideoPostProcess *video_postprocess_init(void) {
	// TODO separate group?
	res_group_preload(NULL, RES_POSTPROCESS, RESF_OPTIONAL, "global", NULL);
	PostprocessShader *pps = res_get_data(RES_POSTPROCESS, "global", RESF_OPTIONAL);

	if(!pps) {
		return NULL;
	}

	auto vpp = ALLOC(VideoPostProcess, {
		.pp_pipeline = pps,
		.mfb_group = fbmgr_group_create(),
	});

	FBAttachmentConfig a = { 0 };
	a.attachment = FRAMEBUFFER_ATTACH_COLOR0;
	a.tex_params.anisotropy = 1;
	a.tex_params.filter.mag = TEX_FILTER_LINEAR;
	a.tex_params.filter.min = TEX_FILTER_LINEAR;
	a.tex_params.mipmap_mode = TEX_MIPMAP_MANUAL;
	a.tex_params.mipmaps = 1;
	a.tex_params.type = TEX_TYPE_RGB_8;
	a.tex_params.wrap.s = TEX_WRAP_MIRROR;
	a.tex_params.wrap.t = TEX_WRAP_MIRROR;

	FramebufferConfig fbconf = { 0 };
	fbconf.num_attachments = 1;
	fbconf.attachments = &a;
	fbconf.resize_strategy.resize_func = fbmgr_resize_strategy_screensized;

	fbmgr_group_fbpair_create(vpp->mfb_group, "Global postprocess", &fbconf, &vpp->framebuffers);
	return vpp;
}

void video_postprocess_shutdown(VideoPostProcess *vpp) {
	if(vpp) {
		fbmgr_group_destroy(vpp->mfb_group);
		mem_free(vpp);
	}
}

Framebuffer *video_postprocess_get_framebuffer(VideoPostProcess *vpp) {
	return vpp ? vpp->framebuffers.front : NULL;
}

static void postprocess_prepare(Framebuffer *fb, ShaderProgram *s, void *arg) {
	VideoPostProcess *vpp = arg;
	r_uniform_int("frames", vpp->frames);
	r_uniform_vec2("viewport", SCREEN_W, SCREEN_H);
}

Framebuffer *video_postprocess_render(VideoPostProcess *vpp) {
	if(!vpp) {
		return NULL;
	}

	postprocess(
		vpp->pp_pipeline,
		&vpp->framebuffers,
		postprocess_prepare,
		draw_framebuffer_tex,
		SCREEN_W,
		SCREEN_H,
		vpp
	);

	++vpp->frames;
	return vpp->framebuffers.front;
}
