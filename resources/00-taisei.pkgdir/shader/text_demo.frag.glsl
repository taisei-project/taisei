#version 330 core

#include "lib/render_context.glslh"
#include "lib/sprite_main.frag.glslh"
#include "lib/util.glslh"

vec3 colormap(float p) {
	vec3 c;
	c.r = smoothstep(0.30, 0.7, p) * smoothstep(0.20, 0.30, 1.0 - p);
	c.g = smoothstep(0.35, 0.7, p) * smoothstep(0.23, 0.32, 1.0 - p);
	c.b = smoothstep(0.35, 0.7, p) * smoothstep(0.24, 0.31, 1.0 - p);
	return c;
}

void spriteMain(out vec4 fragColor) {
	float t = customParams.r;
	vec2 tco = flip_native_to_bottomleft(texCoordOverlay);
	float base_gradient = smoothstep(-0.5, 0.8, tco.y);

	vec4 clr = vec4(
		pow(vec3(base_gradient, base_gradient, base_gradient),
			vec3(1.3, 1.2, 1.1) - 0.5
		), 1);

	float go = tco.y;
	go += tco.x * mix(-1, 1, 1 - tco.y);

	vec4 g = vec4(colormap(fract(-0.5 * t + go)), 0);
	clr = alphaCompose(clr, g);

	vec3 outlines = texture(tex, texCoord).rgb;
	vec4 border = vec4(vec3(g.rgb), 0.5) * outlines.g;
	vec4 fill = clr * outlines.r;

	fragColor = alphaCompose(border, fill);
}
