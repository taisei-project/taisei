#version 330 core

#include "interface/standard.glslh"

uniform(1) int width;
uniform(2) int height;

vec2 tc_normalize(ivec2 tc) {
	return vec2(tc) / vec2(width, height);
}

ivec2 tc_denormalize(vec2 tc) {
	return ivec2(tc * vec2(width, height));
}

vec4 replace_color(vec4 texel, vec4 neighbour) {
	return mix(texel, neighbour, float(neighbour.a > texel.a));
}

vec4 sampleofs(ivec2 origin, int ofsx, int ofsy) {
	return texture(tex, tc_normalize(origin + ivec2(ofsx, ofsy)));
}

void main(void) {
	ivec2 tc = tc_denormalize(texCoordRaw);
	vec4 texel = texture(tex, texCoordRaw);
	vec4 new_texel = texel;

	/*
	 *  GL_LINEAR will sample even pixels with zero alpha.
	 *  Those usually don't have any meaningful RGB data.
	 *  This results in ugly dark borders around some sprites.
	 *  As a workaround, we change the RGB values of such pixels to those of the most opaque nearby one.
	 */

	new_texel = replace_color(new_texel, sampleofs(tc,  0,  1));
	new_texel = replace_color(new_texel, sampleofs(tc,  1,  0));
	new_texel = replace_color(new_texel, sampleofs(tc,  0, -1));
	new_texel = replace_color(new_texel, sampleofs(tc, -1,  0));
	texel = mix(texel, vec4(new_texel.rgb, texel.a), float(texel.a <= 0.0));

	fragColor = texel;
}
