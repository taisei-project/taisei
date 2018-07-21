#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"
#include "lib/util.glslh"

#define NUM_GAPS 4

UNIFORM(1) vec2 viewport;
UNIFORM(2) float time;
UNIFORM(3) vec4 gaps[NUM_GAPS];
UNIFORM(7) float gap_angles[NUM_GAPS];
UNIFORM(11) vec2 gap_flipmasks[NUM_GAPS];

void main(void) {
	vec4 bg = texture(tex, texCoord);
	fragColor = bg;

	vec2 frag_loc = texCoord * viewport;
	frag_loc.y = viewport.y - frag_loc.y;

	float t = time * 6;
	const float mag = 0.5;
	const float pmag = 0.123;
	frag_loc.x += mag * sin(t);
	frag_loc.y += mag * sin(t *  1.22 - frag_loc.x * pmag);
	frag_loc.x += mag * sin(t * -1.36 - frag_loc.y * pmag);
	frag_loc.y += mag * sin(t * -1.29 - frag_loc.x * pmag);
	frag_loc.x += mag * sin(t *  1.35 - frag_loc.y * pmag);

	const float h0 = 1;
	const float h1 = h0 * 0.8;
	const vec4 gap_color = vec4(0.9, 0, 1, 1);

	for(int i = 0; i < 4; ++i) {
		vec4 gap = gaps[i];
		float a = gap_angles[i];
		mat2 m = mat2(cos(a), -sin(a), sin(a), cos(a));
		float edge = length(m * (frag_loc - gap.xy) / gap.zw );
		float gap_mask = smoothstep(h0, h1, edge);

		vec2 tc_inv = 1 - texCoord;
		vec2 tc;
		tc.x = mix(texCoord.x, tc_inv.x, gap_flipmasks[i].x);
		tc.y = mix(texCoord.y, tc_inv.y, gap_flipmasks[i].y);

		fragColor = mix(fragColor, mix(gap_color, texture(tex, tc), 1 - pow(edge, 3)), gap_mask);
	}

	// fragColor = r_color * smoothstep(1, 0.8, length((texCoord - 0.5) * 2));
	// fragColor = bg;
}
