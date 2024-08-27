#version 330 core

#include "lib/render_context.glslh"
#include "lib/sprite_main.frag.glslh"
#include "lib/util.glslh"

float sampleNoise(vec2 tc) {
	tc.y *= fwidth(tc.x) / fwidth(tc.y);
	return texture(tex_aux0, tc).r;
}

void spriteMain(out vec4 fragColor) {
	float t = customParams.x;
	float r = customParams.y;

	vec4 glyph = texture(tex, texCoord);
	float noise = sampleNoise(texCoordOverlay);

	float d = 0.5;
	float x = 1 - t;
	float slope = x;
	float f = smoothstep(0.5 - d * (1.0 - x), 0.5 + d * x, r + (1 - 2 * r) * mix(texCoordOverlay.y, 1.0 - texCoordOverlay.x, slope) - x + 0.5);

	vec4 textfrag = vec4(vec3(glyph.r + 0.25 * glyph.g), glyph.g);
	textfrag *= smoothstep(1 - 2 * f, 1-f, noise);
	textfrag *= color;

	fragColor = textfrag;
}
