#version 330 core

uniform vec3 lightvec;

layout(std140) uniform RenderContext {
	mat4 modelViewMatrix;
	mat4 projectionMatrix;
	mat4 textureMatrix;
	vec4 color;
} ctx;

layout(location=0) in vec3 position;
layout(location=1) in vec3 normalIn;
layout(location=2) in vec2 texCoordRawIn;

out vec2 texCoord;
out vec3 normal;
out vec3 l;

void main(void) {
	gl_Position = ctx.projectionMatrix * ctx.modelViewMatrix * vec4(position, 1.0);
	texCoord = (ctx.textureMatrix * vec4(texCoordRawIn, 0.0, 1.0)).xy;
	normal = normalIn;
	l = lightvec - (ctx.modelViewMatrix*vec4(position,1.0)).xyz;
}
