#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

UNIFORM(1) float time;
UNIFORM(2) sampler2D tex_bg;
UNIFORM(3) sampler2D tex_clouds;
UNIFORM(4) sampler2D tex_swarm;

void main(void) {
	vec2 uv = texCoord;
	float r = 768.0/1024.0;

	vec4 bg = texture(tex_bg, ((uv - 0.5) * vec2(r, 1)) + 0.5);
	vec4 cl0 = texture(tex_clouds, uv + vec2(0.004 * sin(4 * time), 0.35 * time));
	vec4 cl1 = texture(tex_clouds, uv + vec2(0.004 * cos(3 * time), -0.5 * time));
	vec4 sw = vec4(vec3(texture(tex_swarm, uv + vec2(0, -0.75 * time)).r), 1);

	vec4 c0 = bg;
	vec4 c1 = clamp(cl0 - c0, 0, 1);
	vec4 c2 = 0.5 * sw + c1;
	vec4 c3 = clamp(0.4 * cl1 - 0.3*c2, 0, 1);

	fragColor = c3;
}
