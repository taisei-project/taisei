#version 330 core

#include "lib/fxaa.glslh"
#include "interface/fxaa.glslh"
#include "lib/render_context.glslh"

void main(void) {
    gl_Position = r_projectionMatrix * r_modelViewMatrix * vec4(position, 1.0);
    v_rcpFrame = 1.0 / tex_size;
    fxaaCoords(
		texCoordRawIn,
		v_rcpFrame,
		v_coordNW,
		v_coordNE,
		v_coordSW,
		v_coordSE,
		v_coordM
	);
}
