#version 330 core

#include "interface/standard.glslh"

UNIFORM(1) sampler2D alphamap;

UNIFORM(2) int linearize;
UNIFORM(3) int multiply_alpha;
UNIFORM(4) int apply_alphamap;
UNIFORM(5) int write_srgb;

/*
 * Adapted from https://github.com/tobspr/GLSL-Color-Spaces/blob/master/ColorSpaces.inc.glsl
 */

const float SRGB_ALPHA = 0.055;

float srgb_to_linear(float channel) {
	if(channel <= 0.04045) {
		return channel / 12.92;
	} else {
		return pow((channel + SRGB_ALPHA) / (1.0 + SRGB_ALPHA), 2.4);
	}
}

float linear_to_srgb(float channel) {
	if(channel <= 0.0031308) {
		return 12.92 * channel;
	} else {
		return (1.0 + SRGB_ALPHA) * pow(channel, 1.0/2.4) - SRGB_ALPHA;
	}
}

void main(void) {
	fragColor = textureLod(tex, texCoordRaw, 0);

	if(linearize != 0) {
		fragColor.rgb = vec3(
			srgb_to_linear(fragColor.r),
			srgb_to_linear(fragColor.g),
			srgb_to_linear(fragColor.b)
		);
	}

	if(multiply_alpha != 0) {
		fragColor.rgb *= fragColor.a;
	}

	if(apply_alphamap != 0) {
		fragColor.a *= textureLod(alphamap, texCoordRaw, 0).r;
	}

	if(write_srgb != 0) {
		fragColor.rgb = vec3(
			linear_to_srgb(fragColor.r),
			linear_to_srgb(fragColor.g),
			linear_to_srgb(fragColor.b)
		);
	}
}
