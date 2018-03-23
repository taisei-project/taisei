#version 330 core

layout(std140) uniform RenderContext {
	mat4 modelViewMatrix;
	mat4 projectionMatrix;
	mat4 textureMatrix;
	vec4 color;
} ctx;

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoordRawIn;
layout(location = 2) in vec4 instance_pos_delta;

out vec2 texCoord;
out vec2 texCoordRaw;

uniform float timeshift;
uniform float width;
uniform float width_exponent;
uniform int span;

float angle(vec2 v) {
	return atan(v.y, v.x);
}

void main(void) {
	vec2 v = position;

	float t1 = gl_InstanceID - span / 2;
	float tail = span / 1.9;
	float s = -0.75 / pow(tail, 2) * (t1 - tail) * (t1 + tail);

	vec2 pos = instance_pos_delta.xy;
	vec2 d = instance_pos_delta.zw;

	float a = -angle(d);
	mat2 m = mat2(cos(a), -sin(a), sin(a), cos(a));

	v.x *= width * 1.5 * length(d);
	v.y *= width * pow(s, width_exponent);

	gl_Position  = ctx.modelViewMatrix * ctx.projectionMatrix * vec4(m * v + pos, 0.0, 1.0);
	texCoord     = (ctx.textureMatrix * vec4(texCoordRawIn, 0.0, 1.0)).xy;
	texCoordRaw  = texCoordRawIn;
}
