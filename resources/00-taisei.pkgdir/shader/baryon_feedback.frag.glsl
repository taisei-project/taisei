#version 330 core

#include "interface/standard.glslh"
#include "lib/blur/blur5.glslh"
#include "lib/util.glslh"
#include "lib/render_context.glslh"

UNIFORM(1) vec2 blur_resolution;
UNIFORM(2) vec2 blur_direction;
UNIFORM(3) float hue_shift;
UNIFORM(4) float time;

void main(void) {
	vec2 uv = texCoord;
	vec2 ouv = uv;

	uv.x += 0.001 * sin(8 * time + ouv.y * 16.0);
	uv.y += 0.001 * cos(8 * time + ouv.x * 16.0);

	fragColor = sample_blur5(tex, uv, blur_direction / blur_resolution);
	fragColor = r_color * fragColor;

	vec3 hsv = rgb2hsv(fragColor.rgb);
	hsv.x = mod(hsv.x + hue_shift, 1.0);

	fragColor.rgb = hsv2rgb(hsv);
}
