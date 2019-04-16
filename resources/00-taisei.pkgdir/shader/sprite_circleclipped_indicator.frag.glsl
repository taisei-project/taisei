#version 330 core

#include "lib/util.glslh"
#include "lib/sprite_main.frag.glslh"

UNIFORM(1) vec4 back_color;
UNIFORM(2) vec2 origin_ofs;

void spriteMain(out vec4 fragColor) {
	float fill = customParams.r;
	vec4 texel = texture(tex, texCoord);
	vec2 tc = flip_native_to_bottomleft(texCoordRaw) - vec2(0.5) + origin_ofs;

	float x = atan(tc.x, tc.y) - pi * (2.0 * fill - 1.0);

	// Crude anti-aliasing.
	// smoothing = 2*fwidth(x) produces a uniform blur,
	// but causes artifacts at the center.
	float smoothing = 0.2 * step(0.01, fill);
	float f = smoothstep(0.5 + smoothing, 0.5 - 0.001, x + 0.5);

	texel.rgb = mix(vec3(lum(texel.rgb)), texel.rgb, step(1, fill));
	texel *= mix(back_color, color, f);
	fragColor = texel;
}
