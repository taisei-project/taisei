#version 330 core

#include "interface/standard.glslh"
#include "lib/util.glslh"

#define ENABLE_ALPHA_CULLING
// #define DEBUG_ALPHA_CULLING

UNIFORM(1) sampler2D shotlayer;
UNIFORM(2) sampler2D flowlayer;
UNIFORM(3) float time;

float f(float x) {
	x = smoothstep(0.0, 1.0, x * x);
	return x;
}

vec4 f(vec4 v) {
	return vec4(f(v.x), f(v.y), f(v.z), f(v.w));
}

vec4 colormap(vec4 c) {
	float a = f(c.a);
	vec3 hsv = rgb2hsv(c.rgb);
	hsv.y *= (1 - a * a * a * a);
	hsv.z += (a * a * a) * (1 - hsv.z);
	return clamp(vec4(hsv2rgb(hsv), a * 0.5), 0.0, 1.0);
}

void main(void) {
	vec2 uv = texCoord;
	vec4 c = texture(shotlayer, uv);

	#ifdef ENABLE_ALPHA_CULLING
	if(all(lessThan(c, vec4(1.0 / 255.0)))) {
		#ifdef DEBUG_ALPHA_CULLING
		fragColor = vec4(1, 0, 0, 1);
		return;
		#else
		discard;
		#endif
	}
	#endif

	c = colormap(c);

	vec4 m = vec4(vec3(0), 1);
	float t = (time + c.a * 0.2) * 0.1 + 92.123431 + fwidth(c.a);

	m.rgb += texture(flowlayer, uv + 0.0524 * vec2(3.123*sin(t*1.632), t*36.345)).rgb;
	m.rgb += texture(flowlayer, uv + 0.0531 * vec2(t*13.624, -t*49.851)).rgb;
	m.rgb += texture(flowlayer, uv + 0.0543 * vec2(-t*12.931, -t*24.341)).rgb;

	m.rgb = hueShift(clamp(m.rgb / 2.5, 0, 1), time * 0.1 + uv.y);

	fragColor = c * m;
}
