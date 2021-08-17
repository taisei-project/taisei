#version 330 core

#include "interface/imgui.glslh"

void main(void) {
	fragColor = color * texture(tex, texCoord);
}
