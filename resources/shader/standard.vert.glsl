#version 330 core

#include "render_context.glsl"

layout(location=0) in vec3 position;
layout(location=2) in vec2 texCoordRawIn;

out vec2 texCoord;
out vec2 texCoordRaw;

void main(void) {
	gl_Position = ctx.projectionMatrix * ctx.modelViewMatrix * vec4(position, 1.0);
	texCoord = (ctx.textureMatrix * vec4(texCoordRawIn, 0.0, 1.0)).xy;
	texCoordRaw = texCoordRawIn;
}
