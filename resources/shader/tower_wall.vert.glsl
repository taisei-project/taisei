#version 330 core

#include "lib/render_context.glslh"

layout(location=0) in vec3 position;
layout(location=2) in vec2 texCoordRawIn;

out vec3 posModelView;
out vec2 texCoord;

void main(void) {
	vec4 pos_mv = ctx.modelViewMatrix * vec4(position, 1.0);
	gl_Position = ctx.projectionMatrix * pos_mv;
	posModelView = pos_mv.xyz;
	texCoord = (ctx.textureMatrix * vec4(texCoordRawIn, 0.0, 1.0)).xy;
}
