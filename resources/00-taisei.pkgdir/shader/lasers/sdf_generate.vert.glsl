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
	float len = distance(a, b);
	float oversize = sdf_range * 2.0 + max(widthAB.x, widthAB.y);
	vec2 size = vec2(len, 0) + oversize;
	float ang = -angle(b - a);
	mat2 r = mat2(cos(ang), -sin(ang), sin(ang), cos(ang));
	vec2 pos = r * (position.xy * size) + mid;

	coord = pos;
	segment = instance_posAB;
	segment_width = widthAB * 0.5;
	segment_time = mix(timeAB.x, timeAB.y, position.x * size.x / len + 0.5);

	gl_Position  = r_projectionMatrix * vec4(pos, 0.0, 1.0);
}
