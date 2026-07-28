#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/standard.glslh"
#include "extra_bg.glslh"

UNIFORM(1) sampler2D depth_tex;
UNIFORM(2) sampler2D mask_tex;
UNIFORM(3) sampler2D background_tex;
UNIFORM(4) sampler2D background_binary_tex;
UNIFORM(5) sampler2D code_tex;
UNIFORM(6) vec4 code_tex_params;  // (aspect_x, aspect_y, num_segs, 1/num_segs)
UNIFORM(7) float global_dissolve;
UNIFORM(8) float time;

vec4 sample_background(vec2 uv, float r, float t) {
	return sample_background(
		background_tex,
		background_binary_tex,
		code_tex,
		code_tex_params,
		uv,
		r,
		t
	);
}

void main(void) {
	vec4 tower = texture(tex, texCoord);
	float depthval = texture(depth_tex, texCoord).r;
	vec4 masks = texture(mask_tex, texCoord);

	float q = 1 - smoothstep(0.8, 1, depthval);
	q = 1 - sqrt(q);
	float d = 1 - clamp(global_dissolve - 3.5 * q, 0, 1);

	float global_dissolve_phase = smoothstep(d*d, 1.5 * d, masks.a);
	masks.rg = mix(masks.rg, vec2(1, 0), global_dissolve_phase);

	if(masks.g == 1) {
		fragColor = tower;
		gl_FragDepth = depthval;
		return;
	}

	float maskval = masks.r;

	maskval = 0.5 + 0.5 * sin(pi * (maskval + 1.42 * smoothstep(0.5, 1.0, depthval) * masks.b));
	vec4 background = sample_background(texCoord,
		mix(0.2 + maskval*0.8, 1, max(global_dissolve_phase, smoothstep(0.95, 1, depthval))),
		time + 0.5 * (masks.r - 1) + d - masks.g
	);
	maskval = smoothstep(0, 1, maskval + 0.5);
	maskval = smoothstep(0.75, 1.0, maskval);
	maskval = mix(maskval, 1, masks.g);

	maskval = mix(maskval, 0, global_dissolve_phase);

	if(depthval < 1) {
		maskval *= smoothstep(1, 0.995, masks.r) * masks.g;
	}

	vec4 result = background;
	result = alphaCompose(result, tower * maskval);
	depthval = mix(depthval, 0, global_dissolve_phase);
	depthval = mix(depthval, depthval * depthval, (1 - maskval) * 0.5);

	fragColor = result;
	gl_FragDepth = depthval;
}
