#version 330 core

#include "interface/standard.glslh"
#include "lib/blur/blur13.glslh"
#include "lib/util.glslh"

UNIFORM(1) vec2 blur_resolution;
UNIFORM(2) vec2 blur_direction;
UNIFORM(3) vec4 fade;

float reduce(float x) {
	return step(0.0025, x) * x;
}

vec4 reduce(vec4 v) {
	return vec4(
		reduce(v.x),
		reduce(v.y),
		reduce(v.z),
		reduce(v.w)
	);
}

void main(void) {
	vec2 uv = texCoord;
	fragColor = fade * sample_blur13(tex, uv, blur_direction / blur_resolution);
	vec3 hsv = rgb2hsv(fragColor.rgb);
	hsv.y = 1 - (1 - hsv.y) * 0.95;
	fragColor.rgb = hsv2rgb(hsv);
	fragColor = reduce(fragColor);
}
