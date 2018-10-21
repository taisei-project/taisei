#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/reimu_gap.glslh"

void main(void) {
	gl_Position = r_projectionMatrix * r_modelViewMatrix * vec4(position, 1.0);
	texCoord = (r_textureMatrix * vec4(texCoordRawIn, 0.0, 1.0)).xy;

    for(int i = 0; i < NUM_GAPS; ++i) {
		vec2 gap = gaps[i];
		float gap_angle = gap_angles[i];
		int link = gap_links[i];
		vec2 next_gap = gaps[link];
		float next_gap_angle = gap_angles[link];
		vec2 tc = vec2(texCoord.x, 1 - texCoord.y) * viewport;
		tc -= gap.xy;
		tc *= rot((next_gap_angle - gap_angle));
		tc += next_gap.xy;
		tc /= viewport;
		tc.y = 1 - tc.y;
		gap_views[i] = tc;
		gap_rotations[i] = rot(gap_angle);
	}
}
