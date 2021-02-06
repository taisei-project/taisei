#version 330 core

#include "lib/render_context.glslh"
#include "interface/ssr.glslh"
#include "lib/util.glslh"
#include "lib/water.glslh"
#include "lib/frag_util.glslh"

const int steps = 10;


// way to optimize: do projection matrix in the vertex shader
vec3 pos_to_texcoord(vec3 pos) {
	vec4 tmp = r_projectionMatrix * vec4(pos, 1);
	return 0.5*tmp.xyz/tmp.w + vec3(0.5);
}

void main(void) {
	vec2 uv = flip_native_to_bottomleft(texCoord - wave_offset);

	float height = 0.01*warpedNoise(uv * 4, time);

	vec2 dheightduv = dFduv(height, uv);

	mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normalize(normal));
	vec3 n = normalize(tbn*vec3(-dheightduv, 1));

	vec3 reflected_ray = reflect(pos, n);
	reflected_ray /= length(reflected_ray.xy);
	float step_size = 15./steps;

	fragColor = r_color;
	int ihit = 0;

	for(int i = 1; i < steps; i++) {
		vec3 raypos = pos + i * step_size * reflected_ray;
		vec3 raycoord = pos_to_texcoord(raypos);
		float bgdepth = texture(depth, raycoord.xy).r;
		if(bgdepth < raycoord.z) {
			ihit = i;
			break; // sorry gpu
		}
	}

	if(ihit == 0) {
		return;
	}
	for(int j = 0; j < steps; j++) {
		vec3 raypos = pos + (ihit-1+j*1.0/(steps-1)) * step_size * reflected_ray;
		vec3 raycoord = pos_to_texcoord(raypos);
		float bgdepth = texture(depth, raycoord.xy).r;
		if(bgdepth < raycoord.z) {
			fragColor += 0.8 * texture(tex, raycoord.xy);
			break; // sorry gpu
		}
	}
}
