#version 330 core

#include "lib/render_context.glslh"
#include "interface/extra_tower_mask.glslh"
#include "lib/util.glslh"

void main(void) {
	// Unfortunately this produces severe artifacts if computed in the vertex shader
	vec2 uv;
	uv.y = distance(worldPos.xyz, vec3(0, 0, worldPos.z)) / 10 + worldPos.z / 120;
	uv.x = atan(worldPos.x, worldPos.y) + worldPos.z * 0.04;
	uv.x = 0.5 + 0.5 * sin(uv.x);

	vec4 s_mask = texture(tex_mask, uv);
	vec4 s_noise = texture(tex_noise, uv);
	vec4 s_mod = texture(tex_mod, texCoord);

	s_noise.r = fract(s_noise.r + s_mod.r);

	float m = s_noise.r;
	m = 0.5 + 0.5 * sin(  pi*m + time + s_mod.r);
	m = 0.5 + 0.5 * sin(2*pi*m + time - s_mod.r);

	float dissolve_phase = smoothstep(dissolve * dissolve, dissolve, s_noise.r);
	float d = mix(1.0 - s_mask.r, 1.0, dissolve_phase);

	fragColor = vec4(m, d, s_mask.g, s_noise.r);
}
