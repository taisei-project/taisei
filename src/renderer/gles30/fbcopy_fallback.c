/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "fbcopy_fallback.h"
#include "gles30.h"
#include "../glescommon/gles.h"
#include "../gl33/gl33.h"
#include "resource/model.h"

static ShaderProgram *blit_shader;
static struct {
	Uniform *outputs_enabled;
	Uniform *depth_enabled;
	Uniform *input_depth;
	Uniform *input_colors[FRAMEBUFFER_MAX_COLOR_ATTACHMENTS];
} blit_uniforms;

void gles30_fbcopyfallback_init(void) {
	StringBuffer sbuf = {};

	strbuf_printf(&sbuf,
		"#version 300 es\n"
		"precision highp float;\n"
		"precision highp int;\n"
		"uniform int outputs_enabled[%i];\n"
		"uniform int depth_enabled;\n"
		"uniform sampler2D input_depth;\n"
		"layout(location = 0) out vec4 outputs[%i];\n",
		FRAMEBUFFER_MAX_COLOR_ATTACHMENTS, FRAMEBUFFER_MAX_COLOR_ATTACHMENTS
	);

	for(int i = 0; i < FRAMEBUFFER_MAX_COLOR_ATTACHMENTS; ++i) {
		strbuf_printf(&sbuf, "uniform sampler2D input_color%i;\n", i);
	}

	strbuf_cat(&sbuf,
		"void main(void) {\n"
		"	ivec2 tc = ivec2(gl_FragCoord.xy);\n"
	);

	// FIXME: do something about mipmaps?

	for(int i = 0; i < FRAMEBUFFER_MAX_COLOR_ATTACHMENTS; ++i) {
		strbuf_printf(&sbuf,
			"	if(outputs_enabled[%i] != 0) "
					"outputs[%i] = texelFetch(input_color%i, tc, 0);\n", i, i, i);
	}

	strbuf_cat(&sbuf,
		"	if(depth_enabled != 0) "
				"gl_FragDepth = texelFetch(input_depth, tc, 0).r;\n"
		"}\n");

	ShaderSource frag_src = {
		.content = sbuf.start,
		.content_size = sbuf.pos - sbuf.start,
		.lang.lang = SHLANG_GLSL,
		.lang.glsl.version = { 300, GLSL_PROFILE_ES },
		.stage = SHADER_STAGE_FRAGMENT,
	};

	log_info("\n%s\n ", frag_src.content);

	ShaderObject *frag_shobj = r_shader_object_compile(&frag_src);
	strbuf_free(&sbuf);

	if(!frag_shobj) {
		log_fatal("Failed to compile internal blit fragment shader");
	}

	static const char vert_src_str[] =
		"#version 300 es\n"
		"uniform vec4 r_viewport;\n"
		"vec2 verts[4] = vec2[](\n"
		"	vec2( 1.0, -1.0),\n"
		"	vec2( 1.0,  1.0),\n"
		"	vec2(-1.0, -1.0),\n"
		"	vec2(-1.0,  1.0)\n"
		");\n"
		"void main(void) {\n"
		"	gl_Position = vec4(verts[gl_VertexID], 0, 1);\n"
		"}\n";

	ShaderSource vert_src = {
		.content = (char*)vert_src_str,
		.content_size = sizeof(vert_src_str),
		.lang.lang = SHLANG_GLSL,
		.lang.glsl.version = { 300, GLSL_PROFILE_ES },
		.stage = SHADER_STAGE_VERTEX,
	};

	ShaderObject *vert_shobj = r_shader_object_compile(&vert_src);

	strbuf_free(&sbuf);

	if(!vert_shobj) {
		log_fatal("Failed to compile internal blit vertex shader");
	}

	if(!(blit_shader = r_shader_program_link(2, (ShaderObject*[]) { vert_shobj, frag_shobj }))) {
		log_fatal("Failed to link internal blit shader");
	}

	r_shader_object_destroy(vert_shobj);
	r_shader_object_destroy(frag_shobj);

	blit_uniforms.outputs_enabled = NOT_NULL(r_shader_uniform(blit_shader, "outputs_enabled[0]"));
	blit_uniforms.depth_enabled = NOT_NULL(r_shader_uniform(blit_shader, "depth_enabled"));
	blit_uniforms.input_depth = NOT_NULL(r_shader_uniform(blit_shader, "input_depth"));

	for(int i = 0; i < ARRAY_SIZE(blit_uniforms.input_colors); ++i) {
		char tmp[64];
		snprintf(tmp, sizeof(tmp), "input_color%i", i);
		blit_uniforms.input_colors[i] = NOT_NULL(r_shader_uniform(blit_shader, tmp));
	}
}

void gles30_fbcopyfallback_shutdown(void) {
	r_shader_program_destroy(blit_shader);
}

void gles30_fbcopyfallback_framebuffer_copy(Framebuffer *dst, Framebuffer *src, BufferKindFlags flags) {
	int u_outputs_enabled[FRAMEBUFFER_MAX_COLOR_ATTACHMENTS] = {};
	Texture *u_inputs[FRAMEBUFFER_MAX_COLOR_ATTACHMENTS] = {};
	Texture *u_input_depth = NULL;
	bool any_enabled = false;

	if(flags & BUFFER_DEPTH) {
		Texture *depth_src = r_framebuffer_get_attachment(src, FRAMEBUFFER_ATTACH_DEPTH);
		if(depth_src && r_framebuffer_get_attachment(dst, FRAMEBUFFER_ATTACH_DEPTH)) {
			u_input_depth = depth_src;
			any_enabled = true;
		}
	}

	if(flags & BUFFER_COLOR) {
		for(int i = 0; i < FRAMEBUFFER_MAX_COLOR_ATTACHMENTS; ++i) {
			FramebufferAttachment a = FRAMEBUFFER_ATTACH_COLOR0 + i;
			Texture *csrc = r_framebuffer_get_attachment(src, a);
			if(csrc && r_framebuffer_get_attachment(dst, a)) {
				u_inputs[i] = csrc;
				u_outputs_enabled[i] = 1;
				any_enabled = true;
			}
		}
	}

	if(!any_enabled) {
		return;
	}

	FramebufferAttachment outputmap_saved[FRAMEBUFFER_MAX_OUTPUTS];
	r_framebuffer_get_output_attachments(dst, outputmap_saved);

	for(int i = 0; i < FRAMEBUFFER_MAX_COLOR_ATTACHMENTS; ++i) {
		if(u_outputs_enabled[i]) {
			r_framebuffer_set_output_attachment(dst, i, FRAMEBUFFER_ATTACH_COLOR0 + i);
		} else {
			r_framebuffer_set_output_attachment(dst, i, FRAMEBUFFER_ATTACH_NONE);
		}
	}

	FloatRect viewport_saved;
	r_framebuffer_viewport_current(dst, &viewport_saved);
	IntExtent fbsize = r_framebuffer_get_size(dst);
	r_framebuffer_viewport(dst, 0, 0, fbsize.w, fbsize.h);
	r_state_push();

	r_blend(BLEND_NONE);
	r_scissor(0, 0, fbsize.w, fbsize.h);
	r_disable(RCAP_CULL_FACE);

	if(u_input_depth) {
		r_enable(RCAP_DEPTH_TEST);
		r_enable(RCAP_DEPTH_WRITE);
		r_depth_func(DEPTH_ALWAYS);
	} else {
		r_disable(RCAP_DEPTH_TEST);
	}

	r_framebuffer(dst);
	r_shader_ptr(blit_shader);

	r_uniform_int_array(blit_uniforms.outputs_enabled, 0, ARRAY_SIZE(u_outputs_enabled), u_outputs_enabled);
	r_uniform_int(blit_uniforms.depth_enabled, u_input_depth != NULL);
	r_uniform_sampler(blit_uniforms.input_depth, u_input_depth);

	for(int i = 0; i < FRAMEBUFFER_MAX_COLOR_ATTACHMENTS; ++i) {
		r_uniform_sampler(blit_uniforms.input_colors[i], u_inputs[i]);
	}

	// FIXME draw without changing VAO somehow
	r_draw(r_model_get_quad()->vertex_array, PRIM_TRIANGLE_STRIP, 0, 4, 0, 0);

	r_state_pop();
	r_framebuffer_set_output_attachments(dst, outputmap_saved);
	r_framebuffer_viewport_rect(dst, viewport_saved);
}
