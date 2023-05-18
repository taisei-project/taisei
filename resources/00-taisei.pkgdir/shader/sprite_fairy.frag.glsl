#version 330 core

#include "lib/sprite_main.frag.glslh"
#include "lib/render_context.glslh"

void spriteMain(out vec4 fragColor) {
	vec4 fairy = color * texture(tex, texCoord);
	float t = customParams.x;

	if(t >= 1) {
		fragColor = fairy;
		return;
	}

	vec2 maskcoord = 4 * gl_FragCoord.xy / r_viewport.zw + customParams.yz;
	float mask = texture(tex_aux[0], maskcoord).r;
	float m1 = 1 - smoothstep(t, t + 0.2, mask);
	float m2 = 1 - smoothstep(t, t + 0.1, mask);

	vec4 h = 20 * mix(vec4(0, 0, 1, 0), vec4(1, 0.25, 0, 0), m2);
	fragColor = mix(fairy * m1, fairy.a * h, m1 - m2);
}
