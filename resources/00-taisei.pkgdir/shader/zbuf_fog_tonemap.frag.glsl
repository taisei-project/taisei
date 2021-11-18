#version 330 core

#include "lib/util.glslh"
#include "lib/pbr.glslh"
#include "interface/standard.glslh"

UNIFORM(1) sampler2D depth;
UNIFORM(2) float start;
UNIFORM(3) float end;
UNIFORM(4) float exponent;
UNIFORM(5) float curvature;
UNIFORM(6) vec4 fog_color;
UNIFORM(7) vec3 exposure;

void main(void) {
	vec2 pos = vec2(texCoord);

	float d = texture(depth, texCoord).x;
	float z = pow(d + curvature * length(texCoordRaw - vec2(0.5, 0.0)), exponent);
	float f = clamp((end - z) / (end - start), 0.0, 1.0);

	vec4 c = alphaCompose(texture(tex, texCoord), fog_color * (1.0 - f));
	c.rgb = PBR_TonemapUchimura(exposure * c.rgb);
	c.rgb = PBR_GammaCorrect(c.rgb);
	fragColor = c;
}
