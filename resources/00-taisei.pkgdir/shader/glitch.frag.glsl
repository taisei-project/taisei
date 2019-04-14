#version 330 core

#include "interface/standard.glslh"

UNIFORM(1) float frames;
UNIFORM(2) float strength;

float f(float x) {
	x = x * 0.3 - 0.1;
	return floor(5 * cos(floor(20.0 * x) - fract(31.3 * x) * sin(floor(-0.2 * x)))) / 5;
}

void main(void) {
	vec2 p = texCoord;

	float t = frames / 54.3123;
	float ft = f(t);
	float s = f(f(t/3.1 - p.y + 0.15 * f(p.x+t)) + ft);
	float g = f(f(p.y + (frames / 33.2)) + floor(frames / 30.2));
	float pulse = sin(4*t);
	pulse = pulse * pulse * pulse * pulse;

	p.x -= strength * 0.5 * g * s * pulse * ft;
	p = clamp(p, 0.0, 1.0);

	fragColor = texture(tex, p);
}
