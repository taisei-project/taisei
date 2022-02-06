#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/standard.glslh"

UNIFORM(1) float R;
UNIFORM(2) vec2 bg_translation;
UNIFORM(3) vec2 center;
UNIFORM(4) sampler2D tex2;
UNIFORM(5) sampler2D blend_mask;
UNIFORM(6) float inv_aspect_ratio;

const float bmin = 3.0 * sqrt(3.0) / 2.0;

// approximate formula for the real thing I pulled out of my sleeve.
// b is the impact parameter (google) all units are in terms of
// Schwarzschild radii
float bending(float b) {
	float x = b - bmin;
	float a = 2.5;
	float c = 1.136;
	float d = 0.405;
	return pi + 2/b - a*log(tanh(c*pow(x,d)));
}

void main(void) {
	vec2 d = texCoordRaw - center;
	d.y *= inv_aspect_ratio;

	float b = length(d)/R;
	if(b < bmin) {
		discard;
	}

	float theta = angle(d);

	float phi = bending(b);

	vec3 n = vec3(0);
	n.zx = dir(phi);
	n.xy = rot(theta)*n.xy;

	float o = 0.1;

	float step0 = smoothstep(0.2, 0.0, n.x) + smoothstep(-0.6, 0, n.z);
	float step1 = smoothstep(0.2, 0.0, n.x + o) + smoothstep(-0.6, 0, n.z + o);

	float antialiasing = smoothstep(bmin,bmin+0.03,b);

	n.xy += bg_translation;

	vec4 tex1c = texture(tex, n.xy+vec2(0.5,0));
	vec4 tex2c = texture(tex2, n.xy+vec2(0.5));
	float blendfac = texture(blend_mask, n.xy).r;

	fragColor = r_color * mix(tex1c, tex2c, clamp(mix(step1, step0, blendfac), 0, 1));
	fragColor.rgb *= antialiasing;
}
