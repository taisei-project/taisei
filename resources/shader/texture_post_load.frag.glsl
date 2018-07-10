#version 330 core

#include "interface/standard.glslh"

vec4 replace_color(vec4 texel, vec4 neighbour) {
	return mix(texel, neighbour, float(neighbour.a > texel.a));
}

void main(void) {
	vec4 texel = textureLod(tex, texCoordRaw, 0);
	vec4 new_texel = texel;

	/*
	 *  GL_LINEAR will sample even pixels with zero alpha.
	 *  Those usually don't have any meaningful RGB data.
	 *  This results in ugly dark borders around some sprites.
	 *  As a workaround, we change the RGB values of such pixels to those of the most opaque nearby one.
	 */

	new_texel = replace_color(new_texel, textureLodOffset(tex, texCoordRaw, 0, ivec2( 0,  1)));
	new_texel = replace_color(new_texel, textureLodOffset(tex, texCoordRaw, 0, ivec2( 1,  0)));
	new_texel = replace_color(new_texel, textureLodOffset(tex, texCoordRaw, 0, ivec2( 0, -1)));
	new_texel = replace_color(new_texel, textureLodOffset(tex, texCoordRaw, 0, ivec2(-1,  0)));
	texel = mix(texel, vec4(new_texel.rgb, texel.a), float(texel.a <= 0.5));

	fragColor = texel;
}
