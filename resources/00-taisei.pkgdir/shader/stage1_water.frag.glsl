#version 330 core

#include "lib/util.glslh"
#include "lib/frag_util.glslh"
#include "interface/standard.glslh"

UNIFORM(1) float time;
UNIFORM(2) vec4 water_color;
UNIFORM(3) vec2 wave_offset;
UNIFORM(4) sampler2D water_noisetex;

#include "lib/water.glslh"

const vec4 reflection_color = vec4(0.5, 0.8, 0.8, 1.0);
const vec4 wave_color = vec4(0.8, 0.8, 1.0, 1.0);

float vignette(vec2 uv, float threshold, float intensity) {
	uv *= 1.0 - uv.yx;
	return smoothstep(0, threshold, uv.x * uv.y * intensity);
}

// don't try to understand this, it's all fake and horrible

void main(void) {
	vec2 uv_raw = texCoord;
	float v = vignette(uv_raw, 0.75, 20);
	vec2 uv = flip_native_to_bottomleft(texCoord - wave_offset);

	mat2 m = mat2(0, -1,
				  1,  0);

	float n = water_sampleWarpedNoise(uv * 4, m, vec2(0, -0.25 * time));

	vec2 dnduv = v * dFduv(n, uv) * 0.2;
	vec2 dnduv2 = dnduv * dnduv * 0.5;

	vec2 reflect_uv = flip_native_to_bottomleft(texCoord);
	reflect_uv.y += 0.03;
	reflect_uv -= 0.5;
	reflect_uv.x *= 3 / (1 + smoothstep(0.5, 1, gl_FragCoord.z));  //  yikes
	reflect_uv += 0.5;
	reflect_uv.y *= reflect_uv.y;
	reflect_uv += 0.03 * sign(dnduv) * sqrt(abs(dnduv)) * vec2(0.1, 1);  // oof
	reflect_uv = flip_bottomleft_to_native(reflect_uv);

	float reflection_vignette = vignette(clamp(reflect_uv - vec2(0, 0.35), 0, 1), 0.5, 40);
	reflection_vignette *= vignette(reflect_uv, 0.2, 20);

	vec4 surface;

	vec2 dUVdx = dFdx(reflect_uv);
	vec2 dUVdy = dFdy(reflect_uv);

	if(reflection_vignette < 1.0/127.0) {
		surface = water_color;
	} else {
		vec4 reflection = textureGrad(tex, reflect_uv, dUVdx, dUVdy);
		reflection.rgb = mix(reflection.rgb, water_color.rgb, reflection.a * 0.5);
		reflection = reflection * reflection_color * (0.5 * reflection_vignette);
		surface = alphaCompose(water_color, reflection);
	}

	float w = max((dnduv2.y - dnduv2.x * 0.25), 0) * 0.10;
	fragColor = mix(surface, wave_color, w);
}
