#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"
#include "lib/util.glslh"

VARYING(4) vec3 pos;
void main(void) {
	fragColor = 0.3*vec4(0.4*normalize(pos)+vec3(0.6), 0.0);
}
