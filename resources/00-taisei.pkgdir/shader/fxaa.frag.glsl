#version 330 core

#include "lib/fxaa.glslh"
#include "interface/fxaa.glslh"

void main(void) {
    fragColor = vec4(fxaa(
		tex,
		v_rcpFrame,
		v_coordNW,
		v_coordNE,
		v_coordSW,
		v_coordSE,
		v_coordM
	), 1);
    gl_FragDepth = texture(depth, v_coordM).r;
}
