#version 330 core

// #define SPRITE_GLOW_CONTROL
#include "lib/sprite_main.frag.glslh"
#include "lib/util.glslh"

void spriteMain(out vec4 fragColor/*, out vec4 glowColor*/) {
	vec4 texel = texture(tex, texCoord);

	if(texel.a == 0.0) {
		discard;
	}

	vec3 glowHSV = rgb2hsv(color.rgb);
	// glowHSV.x -= 0.1;
	glowHSV.y = 1 - (1 - glowHSV.y) * (1 - glowHSV.y);
	glowHSV.z = 1.0;
	vec3 glowRGB = hsv2rgb(glowHSV);

	fragColor = (vec4(glowRGB, 0) * texel.r + color * texel.g + vec4(texel.b)) * customParams.r;
	// glowColor = vec4(color.rgb * texel.r * customParams.r * 0.4, 0);
}
