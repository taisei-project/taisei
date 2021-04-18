#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"
#include "lib/util.glslh"

UNIFORM(1) sampler2D noise_tex;
UNIFORM(2) sampler2D paper_tex;
UNIFORM(3) sampler2D erase_mask_tex;
UNIFORM(4) float distort_strength;
UNIFORM(5) vec2 thresholds;

float render_fademap(vec2 uv, vec2 distort) {
	// Technically this could be pre-rendered, but 8 bits of precision is not enough,
	// and renderdoc won't let me save a 16-bit texture for some dumb reason, so whatever.

	vec2 ntc_n = (uv - 0.5) + (distort * 3) / 0.2;
	vec2 ntc = 0.2 * ntc_n + vec2(0.23, 0.43);
	float n = texture(noise_tex, ntc).r;
	float r = 1 - 2 * length(ntc_n);
	float f = r * r * 0.4 + n * 0.6;

	// hardcoded range adjustment; crucify me
	f -= 0.0012944491858334999;
	f *= 1.559332605644784;
	f = clamp(f, 0, 1);

	return f;
}

float project_drawing(float drawing, float fademap, float grainmap) {
	return 1 - smoothstep(0.0, 1.0, fademap * grainmap) * (1 - drawing);
}

void main(void) {
	vec4 paper = texture(paper_tex, texCoord);
	vec2 distort_offset = vec2(-0.7, -0.75);  // NOTE: tuned for the paper texture
	vec2 distort = (vec2(paper.g, paper.r) + distort_offset) * distort_strength;

	float fademap = render_fademap(texCoord, distort);
	fragColor = vec4(vec3(fademap), 1);
	fragColor.gb = mod(fragColor.gb, vec2(2.0));

	float drawing = texture(tex, texCoord + distort).r;

	float erase_mask = texture(erase_mask_tex, texCoord + distort * 10).a;
	erase_mask = smoothstep(0, 0.3, erase_mask * (paper.b + 1));
	drawing = min(1, drawing + erase_mask);

	vec2 fade_thresholds = pow(thresholds, 1 / vec2(paper.b));
	fademap = smoothstep(fade_thresholds.x, fade_thresholds.y, fademap);

	float grainmap = pow(smoothstep(0, 0.9, paper.r), 4.0);

	float fademap2 = fademap * fademap;
	float fademap3 = fademap2 * fademap;
	float fademap5 = fademap3 * fademap2;

	float pd = 0;
	pd += project_drawing(drawing, fademap,  grainmap) * 0.4;
	pd += project_drawing(drawing, fademap3, grainmap) * 0.3;
	pd += project_drawing(drawing, fademap5, grainmap) * 0.3;
	drawing = pd;

	vec3 color = mix(paper.rgb * paper.rgb * 0.34, paper.rgb, drawing);

	fragColor = vec4(color, 1);
}
