#version 330 core

#include "../lib/render_context.glslh"
#include "../lib/util.glslh"
#include "../interface/laser_pass2.glslh"

void main(void) {
	vec2 midpoint = instance_bbox.xy;
	vec2 size = instance_bbox.zw;

	bool rotated = size.x < 0;

	if(rotated) {
		size.x = -size.x;
	}

	vec2 vert_pos = vertex_position * size + midpoint;
	vec2 vert_tc = flip_native_to_topleft(vertex_uv);
	vert_tc = vert_tc * size;

	if(rotated) {
		vert_tc = vert_tc.yx;
	}

	vert_tc = (vert_tc + instance_texofs) / texsize;
	vert_tc = flip_topleft_to_native(vert_tc);

	gl_Position = r_projectionMatrix * r_modelViewMatrix * vec4(vert_pos, 0.0, 1.0);
	texCoord = vert_tc;
	color_width = instance_color_width;
}
