#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"
#include "extra_bg.glslh"

UNIFORM(1) sampler2D background_tex;
UNIFORM(2) sampler2D background_binary_tex;
UNIFORM(3) sampler2D code_tex;
UNIFORM(4) vec4 code_tex_params;
UNIFORM(5) float time;

void main(void) {
    fragColor = sample_background(
		background_tex,
		background_binary_tex,
		code_tex,
		code_tex_params,
		texCoord,
		1,
		time
	);
}
