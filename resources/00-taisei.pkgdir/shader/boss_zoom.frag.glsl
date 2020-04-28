#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

UNIFORM(1) vec2 blur_orig; // center
UNIFORM(2) vec2 fix_orig;
UNIFORM(3) float blur_rad;  // radius of zoom effect
UNIFORM(4) float rad;
UNIFORM(5) float ratio; // texture h/w
UNIFORM(6) vec4 color;

vec3 sampleZoom(vec3 ca) {
	vec2 pos = texCoordRaw - blur_orig;
	vec2 posProportional = pos * vec2(1, ratio);

	vec3 z = ca * vec3(length(posProportional) / blur_rad);
	z = 1.5 * z * z * z;
	z = tanh(z);
	z = sqrt(z);

	vec2 posR = clamp(pos * z.r + blur_orig, 0.005, 0.995);
	vec2 posG = clamp(pos * z.g + blur_orig, 0.005, 0.995);
	vec2 posB = clamp(pos * z.b + blur_orig, 0.005, 0.995);

	return vec3(
		texture(tex, posR).r,
		texture(tex, posG).g,
		texture(tex, posB).b
	);
}

vec3 getCA(float f) {
	return (1.0 + f * vec3(1, 2, 3));
}

void main(void) {
	fragColor.rgb = sampleZoom(getCA(0.035));
	fragColor.a = 1.0;

	vec2 pos = (texCoordRaw - fix_orig) * vec2(1, ratio);
	fragColor *= pow(color, vec4(3.0 * max(0.0, rad - length(pos))));
}
