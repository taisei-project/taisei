#version 330 core

#include "lib/util.glslh"
#include "interface/standard.glslh"

UNIFORM(1) float time;
UNIFORM(2) vec4 water_color;
UNIFORM(3) vec2 wave_offset;

const vec4 reflection_color = vec4(0.5, 0.8, 0.8, 1.0);
const vec4 wave_color = vec4(0.8, 0.8, 1.0, 1.0);

// Based on https://www.shadertoy.com/view/Xl2XWz

float smoothNoise(vec2 p) {
	vec2 f = fract(p);
	p -= f;
	f *= f * (3 - f - f);

	// Intel's sin() function breaks down with values over 1e4 (yes, really)
	// get rid of some bits here w/o sacrificing the nice effect
	vec4 a = vec4(0, 1, 27, 28) + p.x + p.y * 27;
	vec4 b = mod(a, tau);

	// WARNING: Some versions of the Windows AMD driver choke on temp_mat = mat2(temp_vec)
	vec4 temp_vec = fract(sin(b) * 1e5);
	mat2 temp_mat = mat2(temp_vec.x, temp_vec.y, temp_vec.z, temp_vec.w);

	return dot(temp_mat * vec2(1 - f.y, f.y), vec2(1 - f.x, f.x));
}

float fractalNoise(vec2 p) {
	return
		smoothNoise(p)     * 0.5333 +
		smoothNoise(p * 2) * 0.2667 +
		smoothNoise(p * 4) * 0.1333 +
		smoothNoise(p * 8) * 0.0667;
}

float warpedNoise(vec2 p) {
	vec2 m = vec2(0.0, -time);
	float x = fractalNoise(p + m);
	float y = fractalNoise(p + m.yx + x);
	float z = fractalNoise(p - m - x);
	return fractalNoise(p + vec2(x, y) + vec2(y, z) + vec2(z, x) + length(vec3(x, y, z)) * 0.1);
}

void main(void) {
	vec2 uv = flip_native_to_bottomleft(texCoord - wave_offset);

	float n = warpedNoise(uv * 4);

	vec2 duvdx = dFdx(uv);
	vec2 duvdy = dFdy(uv);
	mat2 duvdxy = mat2(duvdx,duvdy);

	float dndx = dFdx(n);
	float dndy = dFdy(n);

	vec2 dnduv = inverse(duvdxy)*vec2(dndx,dndy)*0.15;
	vec2 dnduv2 = dnduv * dnduv * 0.5;

	uv = flip_native_to_bottomleft(texCoord);
	uv += 0.25 * dnduv2;
	uv = flip_bottomleft_to_native(uv);

	vec4 reflection = texture(tex, uv);
	reflection.rgb = mix(reflection.rgb, water_color.rgb, reflection.a * 0.5);
	reflection = reflection * reflection_color * 0.5;
	vec4 surface = alphaCompose(water_color, reflection);

	fragColor = mix(surface, wave_color, max((dnduv2.y-dnduv2.x*0.25),0) * 0.10);
}
