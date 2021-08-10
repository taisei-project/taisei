#version 330 core

#include "../lib/render_context.glslh"
#include "../lib/util.glslh"
#include "../interface/laser_pass1.glslh"

void main(void) {
	vec2 a = instance_posAB.xy;
	vec2 b = instance_posAB.zw;
	vec2 widthAB = instance_widthAB_timeAB.xy;
	vec2 timeAB = instance_widthAB_timeAB.zw;

	vec2 mid = (a + b) * 0.5;
	vec2 size = vec2(distance(a, b), 0) + (sdf_range * 2.0 + max(widthAB.x, widthAB.y));
	float ang = -angle(b - a);
	mat2 r = mat2(cos(ang), -sin(ang), sin(ang), cos(ang));
	vec2 pos = r * (position.xy * size) + mid;

	coord = pos;
	segment = instance_posAB;
	segment_width = widthAB * 0.5;
	segment_time = mix(timeAB.x, timeAB.y, position.x + 0.5);

	gl_Position  = r_projectionMatrix * r_modelViewMatrix * vec4(pos, 0.0, 1.0);
}
