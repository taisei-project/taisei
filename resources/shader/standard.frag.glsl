#version 330 core

layout(std140) uniform RenderContext {
	mat4 modelViewMatrix;
	mat4 projectionMatrix;
	mat4 textureMatrix;
	vec4 color;
} ctx;

uniform sampler2D tex;

in vec2 texCoord;
out vec4 fragColor;

void main(void) {
	fragColor = ctx.color * texture(tex, texCoord);
}
