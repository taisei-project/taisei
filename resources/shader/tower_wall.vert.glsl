#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

VARYING(3) vec3 posModelView;

void main(void) {
	vec4 pos_mv = r_modelViewMatrix * vec4(position, 1.0);
	gl_Position = r_projectionMatrix * pos_mv;
	posModelView = pos_mv.xyz;
	texCoord = (r_textureMatrix * vec4(texCoordRawIn, 0.0, 1.0)).xy;
}
