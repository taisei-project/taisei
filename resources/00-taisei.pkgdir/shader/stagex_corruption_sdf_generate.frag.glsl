#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/standard.glslh"

#define MAX_ZONES 128

UNIFORM(1) float sdf_range;
UNIFORM(2) int num_zones;
UNIFORM(3) vec2 viewport;
UNIFORM(4) vec4 zones[MAX_ZONES];

void main(void) {
	vec2 pos = viewport.xy * vec2(texCoord.x, texCoord.y);
	vec2 dist = vec2(sdf_range);

	for(int i = 0; i < num_zones; ++i) {
		float zone_dist = distance(pos, zones[i].xy) - zones[i].z;

		vec2 mindist = vec2(
			smoothmin(zone_dist, dist.x, 42),
			min(zone_dist, dist.y)
		);

		dist = mix(dist, mindist, zones[i].w);
	}

	// dist = tanh(dist) * 2 * sdf_range - sdf_range;

	fragColor = vec4(dist, 0, 0);
}
