#version 430 core

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform vec4 color;

void main(void) {
	// https://github.com/KhronosGroup/glslang/issues/2206
    float foo = 42;
    foo = smoothstep(0, 1, foo);
    outColor = texture(tex, texCoord) * color * foo;
}
