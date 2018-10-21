#version 330 core

#include "lib/render_context.glslh"
#include "interface/reimu_gap.glslh"
#include "lib/util.glslh"

// #define DRAW_LINKS

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
	const vec4 gap_color = vec4(0.75, 0, 0.4, 1);

	for(int i = 0; i < NUM_GAPS; ++i) {
		vec2 gap = gaps[i];
		mat2 gap_rot = gap_rotations[i];
		float edge = length(gap_rot * (frag_loc - gap) / gap_size);
		float gap_mask = smoothstep(h0, h1, edge);
		vec2 tc = gap_views[i];
		fragColor = mix(fragColor, mix(gap_color, texture(tex, tc), 1 - pow(edge, 3)), gap_mask);
	}

	#ifdef DRAW_LINKS
	for(int i = 0; i < NUM_GAPS; ++i) {
		vec2 gap = gaps[i];
		float gap_angle = gap_angles[i];
		int link = gap_links[i];

		vec2 next_gap = gaps[link];
		float next_gap_angle = gap_angles[link];

		vec2 l = texCoord * viewport;
		l.y = viewport.y - l.y;

		fragColor = mix(fragColor, vec4(float(i&1), i/float(NUM_GAPS-1), 1-i/float(NUM_GAPS-1), 1), line_segment(l, gap, next_gap, 1));
	}
	#endif
}
