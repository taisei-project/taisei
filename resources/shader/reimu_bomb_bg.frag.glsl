#version 330 core

#include "interface/standard.glslh"
#include "lib/util.glslh"
#include "lib/render_context.glslh"

UNIFORM(1) vec2 aspect;
UNIFORM(2) float zoom;
UNIFORM(3) float time;
UNIFORM(4) sampler2D runes;

float _split(float x) {
	return sqrt(0.25 - pow(x - 0.5, 2));
}

vec2 runes_transform_uv(vec2 uv) {
	const float segments = 32;
	uv.y = mod(uv.y, 1.0);
	uv.y += floor(uv.x);
	uv.y /= segments;
	return uv;
}

float split(float x) {
/*
	if(x <= 0.25) {
		return 0.25;
	}

	if(x >= 0.75) {
		return 0.75;
	}
*/

	if(x < 0.5) {
		x = 1.0 - _split(2 * x) - 0.5;
	} else {
		x = _split(2 * x - 1) + 0.5;
	}

	return 0.5 * x + 0.25;
}

float yinyang(vec2 uv, out float light_dist, out float dark_dist) {
	float s = split(uv.x);
	float v;

	v = smoothstep(0.0, 0.01, uv.y - s);
	light_dist = length((uv - vec2(0.75, 0.5)) * 16.0);
	v = mix(v, 1.0, smoothstep(1.0, 0.95, light_dist));
	dark_dist = length((uv - vec2(0.25, 0.5)) * 16.0);
	v = mix(v, 0.0, smoothstep(1.0, 0.95, dark_dist));

	return v;
}

float yinyang_spin(vec2 uv, vec2 aspect, float a, out float light_dist, out float dark_dist) {
	mat2 r = rot(a);
	uv -= 0.5;
	uv *= aspect;
	uv *= r;
	uv += 0.5;
	return yinyang(uv, light_dist, dark_dist);
}

void main(void) {
	vec2 uv = texCoord;
	float ld, dd;

	float yy = yinyang_spin(uv, aspect * zoom, -time, ld, dd);

	vec2 uv_r = uv;
	vec2 uv_g = uv;
	vec2 uv_b = uv;

	vec3 c = vec3(0);
	float rfactor = sqrt(dd*dd + ld*ld) * 0.01 * yy;
	float kek = (1 - 2 * yy);

	ld = sqrt(ld);
	dd = sqrt(dd);

	mat2 m_r = rot(dd * 0.00005 * kek) / (1 + ld * -0.0006 * kek);
	mat2 m_g = rot(dd * 0.00009 * kek) / (1 + ld * -0.0004 * kek);
	mat2 m_b = rot(dd * 0.00012 * kek) / (1 + ld * -0.0002 * kek);

	const int samples = 24;
	const float isamples = 1.0 / samples;

	for(int i = 0; i < samples; ++i) {
		uv_r -= 0.5;
		uv_r *= m_r;
		uv_r += 0.5;
		c.r += texture(tex, uv_r).r;

		uv_g -= 0.5;
		uv_g *= m_g;
		uv_g += 0.5;
		c.g += texture(tex, uv_g).g;

		uv_b -= 0.5;
		uv_b *= m_b;
		uv_b += 0.5;
		c.b += texture(tex, uv_b).b;
	}

	fragColor = vec4(c * isamples, 1) * 0.9 * mix(r_color, vec4(r_color.a), 1);

	const float rune_lines = 24;

	vec2 runes_uv = (uv - 0.5) * rot(pi * 0.5) + 0.5;
	float line = floor(runes_uv.y * rune_lines);
	runes_uv.x += 0.931223 * line + time * 0.1 * cos(pi * line);
	runes_uv.y *= -rune_lines;
	runes_uv = runes_transform_uv(runes_uv);

	float rune_factor = texture(runes, runes_uv).r;
	fragColor.rgb = pow(fragColor.rgb, vec3(1 + (1 - 2 * rune_factor) * 0.1));
	fragColor.a *= (1 - rune_factor * 0.1);
}
