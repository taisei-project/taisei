#version 330 core

#include "interface/standard.glslh"
#include "lib/util.glslh"

#define POINTS 120
// #define INTERPOLATE

UNIFORM(1) vec3 color_low;
UNIFORM(2) vec3 color_mid;
UNIFORM(3) vec3 color_high;
UNIFORM(4) float points[POINTS];

float get_sample(float x) {
	x = clamp(x, 0.0, 1.0) * float(POINTS - 1);

	float f = floor(x);
	float y0 = points[int(f)];

#ifdef INTERPOLATE
	float c = ceil(x);
	float y1 = points[int(c)];
	float fract = x - f;
	return y1 * fract + y0 * (1.0 - fract);
#else
	return y0;
#endif
}

void main(void) {
	vec4 c = vec4(1.0);
	vec2 coord = vec2(texCoord.x, 1.0 - texCoord.y); // TODO: move to vertex shader
	float s = get_sample(coord.x);
	
	c.a = float(coord.y <= s);
	c.a *= (0.2 + 0.8 * s);
	c.a = 0.05 + 0.95 * c.a;

	float cval = s; // coord.y;

	if(cval > 0.5) {
		c.rgb = mix(color_mid, color_high, 2.0 * (cval - 0.5));
	} else {
		c.rgb = mix(color_low, color_mid,  2.0 *  cval);
	}

	c = mix(c, vec4(1.0), 0.5 * pow(1.0 - abs(coord.y - 0.5), 32.0));

	fragColor = color_mul_alpha(c);
}
