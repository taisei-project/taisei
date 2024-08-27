#version 330 core

#include "lib/sprite_main.frag.glslh"
#include "lib/render_context.glslh"
#include "lib/util.glslh"

vec2 extract_vec2(float x) {
	uint bits = floatBitsToUint(customParams.z);
	// NOTE: could use unpackUnorm2x16 in GLSL 410
	return vec2(bits & 0xFFFFu, bits >> 16) / float((1 << 16) - 1);
}

void spriteMain(out vec4 fragColor) {
	vec4 fairy = color * texture(tex, texCoord);
	float p_cloak = customParams.y;

	if(p_cloak == 0) {
		fragColor = fairy;
		return;
	}

	float p_summon = customParams.x;
	vec2 noise_ofs = extract_vec2(customParams.z);
	float globaltime = customParams.w;

	vec2 base_noise_coord = gl_FragCoord.xy / r_viewport.zw;

	vec2 clknoise_coord = vec2(1, 20) * base_noise_coord + noise_ofs + vec2(globaltime);
	float clk = fairy.a * texture(tex_aux0, clknoise_coord).r;
	vec3 n = vec3(0.5, 0.7, 1.0) * clk;
	fairy.rgb = mix(fairy.rgb, lum(fairy.rgb) * n, p_cloak);

	vec2 maskcoord = 4 * base_noise_coord + noise_ofs;
	float mask = texture(tex_aux0, maskcoord).r;
	float m1 = 1 - smoothstep(p_summon, p_summon + 0.2, mask);
	float m2 = 1 - smoothstep(p_summon, p_summon + 0.1, mask);

	vec4 h = 20 * mix(vec4(0, 0, 1, 0), vec4(1, 0.25, 0, 0), m2);
	fragColor = mix(fairy * m1, fairy.a * h, m1 - m2);
}
