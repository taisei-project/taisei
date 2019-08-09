/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "events.h"
#include "log.h"
#include "resource/postprocess.h"
#include "util/graphics.h"
#include "video.h"
#include "video_postprocess.h"

struct VideoPostProcess {
	FBPair framebuffers;
	PostprocessShader *pp_pipeline;
	int frames;
};

static bool video_postprocess_resize_event(SDL_Event *e, void *arg) {
	VideoPostProcess *vpp = arg;
	float w, h;
	video_get_viewport_size(&w, &h);
	fbpair_resize_all(&vpp->framebuffers, w, h);
	fbpair_viewport(&vpp->framebuffers, 0, 0, w, h);
	return false;
}

VideoPostProcess *video_postprocess_init(void) {
	PostprocessShader *pps = get_resource_data(RES_POSTPROCESS, "global", RESF_OPTIONAL | RESF_PERMANENT | RESF_PRELOAD);

	if(!pps) {
		return NULL;
	}

	VideoPostProcess *vpp = calloc(1, sizeof(*vpp));
	vpp->pp_pipeline = pps;

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

	float w, h;
	video_get_viewport_size(&w, &h);
	a.tex_params.width = w;
	a.tex_params.height = h;

	fbpair_create(&vpp->framebuffers, 1, &a, "Global postprocess");
	fbpair_viewport(&vpp->framebuffers, 0, 0, w, h);

	r_framebuffer_clear(vpp->framebuffers.back, CLEAR_ALL, RGBA(0, 0, 0, 0), 1);
	r_framebuffer_clear(vpp->framebuffers.front, CLEAR_ALL, RGBA(0, 0, 0, 0), 1);

	events_register_handler(&(EventHandler) {
		.proc = video_postprocess_resize_event,
		.priority = EPRIO_SYSTEM,
		.arg = vpp,
		.event_type = MAKE_TAISEI_EVENT(TE_VIDEO_MODE_CHANGED),
	});

	return vpp;
}

void video_postprocess_shutdown(VideoPostProcess *vpp) {
	if(vpp) {
		events_unregister_handler(video_postprocess_resize_event);
		fbpair_destroy(&vpp->framebuffers);
		free(vpp);
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
