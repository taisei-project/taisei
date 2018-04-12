#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

VARYING(3) vec3 posRaw;

void main(void) {
	gl_Position = r_projectionMatrix * r_modelViewMatrix * vec4(position, 1.0);
	posRaw = position;
	texCoord = (r_textureMatrix * vec4(texCoordRawIn, 0.0, 1.0)).xy;
}
