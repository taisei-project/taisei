#version 330 core

#include "lib/util.glslh"
#include "lib/water.glslh"
#include "lib/frag_util.glslh"
#include "interface/standard.glslh"

UNIFORM(1) float time;
UNIFORM(2) vec4 water_color;
UNIFORM(3) vec2 wave_offset;

const vec4 reflection_color = vec4(0.5, 0.8, 0.8, 1.0);
const vec4 wave_color = vec4(0.8, 0.8, 1.0, 1.0);

void main(void) {
	vec2 uv = flip_native_to_bottomleft(texCoord - wave_offset);

	float n = warpedNoise(uv * 4, time);



	vec2 dnduv = dFduv(n, uv)*0.15;
	vec2 dnduv2 = dnduv * dnduv * 0.5;

	uv = flip_native_to_bottomleft(texCoord);
	uv += 0.25 * dnduv2;
	uv = flip_bottomleft_to_native(uv);

	vec4 reflection = texture(tex, uv);
	reflection.rgb = mix(reflection.rgb, water_color.rgb, reflection.a * 0.5);
	reflection = reflection * reflection_color * 0.5;
	vec4 surface = alphaCompose(water_color, reflection);

	fragColor = mix(surface, wave_color, max((dnduv2.y-dnduv2.x*0.25),0) * 0.10);
}
