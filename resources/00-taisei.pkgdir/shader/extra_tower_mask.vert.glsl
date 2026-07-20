#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/extra_tower_mask.glslh"

void main(void) {
	vec4 pos4 = vec4(position, 1.0);
	vec4 camPos4 = r_modelViewMatrix * pos4;
	camPos = camPos4.xyz;
	worldPos = (world_from_model * pos4).xyz;

	gl_Position = r_projectionMatrix * camPos4;
	texCoord = texCoordRawIn;

	modelPos = position;
}
