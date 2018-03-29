#version 330 core

uniform sampler2D tex;
uniform int width;
uniform int height;

in vec2 texCoord;
in vec2 texCoordRaw;
out vec4 fragColor;

ivec2 tc_denormalize(vec2 tc) {
	return ivec2(tc * vec2(width, height));
}

vec4 replace_color(vec4 texel, vec4 neighbour) {
	return mix(texel, neighbour, float(neighbour.a > texel.a));
}

vec4 sampleofs(ivec2 origin, int ofsx, int ofsy) {
	return texelFetch(tex, origin + ivec2(ofsx, ofsy), 0);
}

void main(void) {
	ivec2 tc = tc_denormalize(texCoordRaw);
	vec4 texel = texelFetch(tex, tc, 0);
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
