#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

UNIFORM(1) vec3 times;
UNIFORM(2) sampler2D mask;

const vec3 cutoffs = vec3(0.53, 0.56, 0.51);
const vec2 grid = vec2(8.0, 16.0);
const vec3 waverectfactors = vec3(7.0);

float hash(vec2 co) {
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt = dot(co, vec2(a, b));
	float sn = mod(dt, 3.14);
	return fract(sin(sn) * c);
}

float wave(float x) {
	float y = sin(x) + cos(3.0*x) + sin(5.0*x) + cos(7.0*x);
	float m = 3.0;
	return clamp((m + y) / (2.0 * m), 0.0, 1.0);
}

vec3 wave(vec3 v) {
	return vec3(
		wave(v.x),
		wave(v.y),
		wave(v.z)
    );
}

float rect(float x, float g) {
	return round(g * x) / g;
}

vec2 rect(vec2 v, vec2 g) {
	return vec2(
		rect(v.x, g.x),
		rect(v.y, g.y)
	);
}

vec3 rect(vec3 v, vec3 g) {
	return vec3(
		rect(v.x, g.x),
		rect(v.y, g.y),
		rect(v.z, g.z)
	);
}

vec3 glitch(vec2 uv, vec3 time, vec2 grid, vec3 cutoffs, vec3 waverectfactors) {
	uv = rect(uv, grid);
	vec3 t = time;
	vec3 v = wave(t);
	vec3 mstatic = step(cutoffs, v);
	vec3 mchaotic = step(cutoffs, v * v);
    v = rect(v, waverectfactors);
	vec3 mask = vec3(
		hash(uv + t.r * mchaotic.r),
		hash(uv + t.g * mchaotic.g),
		hash(uv + t.b * mchaotic.b)
	);
	mask = step(mstatic * v, mask);
	mask = (1.0 - mask);
	// mask *= v;
	return mask;
}

void main(void) {
	vec2 uv = texCoord;

	vec4 texNormal = texture(tex, uv);
	vec3 m = texture(mask, uv).rgb;
	float a = m.r + m.g + m.b;

	if(a == 0) {
		fragColor = texNormal;
		return;
	}

	vec3 k = glitch(uv, times, grid, cutoffs, waverectfactors);
	vec3 o = k * a * 0.25;

	vec3 texGlitched;
	texGlitched.r = texture(tex, uv - o.r / grid).r;
	texGlitched.g = texture(tex, uv - o.g / grid).g;
	texGlitched.b = texture(tex, uv - o.b / grid).b;

	fragColor.rgb = mix(texNormal.rgb, texGlitched, k);
	fragColor.a = texNormal.a;
}
