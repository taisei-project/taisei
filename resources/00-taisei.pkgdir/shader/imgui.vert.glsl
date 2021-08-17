#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/imgui.glslh"

void main(void) {
	gl_Position = r_projectionMatrix * vec4(position, 0.0, 1.0);
	texCoord = flip_topleft_to_native(texCoordRawIn);
	color = colorIn * colorIn.a;
}
