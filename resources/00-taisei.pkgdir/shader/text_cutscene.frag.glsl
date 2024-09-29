#version 330 core

#include "lib/sprite_main.frag.glslh"

float sampleNoise(vec2 tc) {
	tc.y *= fwidth(tc.x) / fwidth(tc.y);
	return texture(tex_aux0, tc).r;
}

void spriteMain(out vec4 fragColor) {
    float mask = sampleNoise(texCoordOverlay);
	float o = customParams.r;
	float xpos = 0.5 + texCoordOverlay.x;
	float slide_factor = 8;
	mask = 1.0 - smoothstep(slide_factor * o * o * 0.95, slide_factor * o, mask + (slide_factor - 1.0) * xpos);
	mask = smoothstep(0.2, 0.8, mask);

	vec3 outlines = texture(tex, texCoord).rgb;

	vec4 highlight = color;
	vec4 fill      = vec4(color.rgb * 0.9, color.a) * mask;
	vec4 border    = vec4(color.rgb * 0.3, color.a) * mask * mask;

	fragColor = outlines.g * mix(border, mix(fill, highlight, outlines.b), outlines.r);
	fragColor.rgb *= mask * mask;
	fragColor *= mask;
}
