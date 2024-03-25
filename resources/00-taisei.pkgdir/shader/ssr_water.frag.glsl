#version 330 core

#include "lib/render_context.glslh"
#include "interface/ssr.glslh"
#include "lib/util.glslh"
#include "lib/frag_util.glslh"

UNIFORM(5) float wave_height;
UNIFORM(6) sampler2D water_noisetex;

#include "lib/water.glslh"

const int steps = 10;

// way to optimize: do projection matrix in the vertex shader
vec3 pos_to_texcoord(vec3 pos) {
	vec4 tmp = r_projectionMatrix * vec4(pos, 1);
	return 0.5*tmp.xyz/tmp.w + vec3(0.5);
}

vec3 trace_screenspace_reflection(vec3 pos, vec3 n, sampler2D screen_depth, sampler2D screen_color) {
	vec3 reflected_ray = reflect(pos, n);
	reflected_ray /= length(reflected_ray.xy);
	const float step_size = 15.0 / steps;

	vec3 color = vec3(0);
	int ihit = 0;

	for(int i = 1; i < steps; i++) {
		vec3 raypos = pos + i * step_size * reflected_ray;
		vec3 raycoord = pos_to_texcoord(raypos);
		float bgdepth = texture(screen_depth, raycoord.xy).r;
		if(bgdepth < raycoord.z) {
			ihit = i;
			break; // sorry gpu
		}
	}

	if(ihit == 0) {
		return color;
	}

	for(int j = 0; j < steps; j++) {
		vec3 raypos = pos + (ihit-1+j*1.0/(steps-1)) * step_size * reflected_ray;
		vec3 raycoord = pos_to_texcoord(raypos);
		float bgdepth = texture(screen_depth, raycoord.xy).r;
		if(bgdepth < raycoord.z) {
			color += 0.8 * texture(screen_color, raycoord.xy).rgb;
			break; // sorry gpu
		}
	}

	return color;
}

void main(void) {
	vec2 uv = flip_native_to_bottomleft(texCoord - wave_offset);
	float height = wave_height * water_sampleWarpedNoise(uv * 4, mat2(2), vec2(time, 0));
	vec2 dheightduv = dFduv(height, uv);

	mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normalize(normal));
	vec3 n = normalize(tbn*vec3(-dheightduv, 1));

	fragColor = r_color;
	fragColor.rgb *= trace_screenspace_reflection(pos, n, depth, tex);
}
