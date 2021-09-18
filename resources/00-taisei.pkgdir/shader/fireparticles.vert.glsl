#version 330 core

#include "lib/defs.glslh"
#include "lib/util.glslh"
#include "lib/render_context.glslh"
#include "lib/hash.glslh"
#include "interface/particle_emitter.glslh"

const float min_scale = 0.8;
const float max_scale = 1.0;
const float min_shrink = 0.2;
const float min_lift = 0.9;
const float max_lift = 1.2;
const float min_ofs = 0.02;
const float max_ofs = 0.2;

const float lift_scale = 3;

const vec3 v_forward = vec3(0, 0, 1);
const vec3 v_right = vec3(1, 0, 0);
const vec3 v_up = vec3(0, -1, 0);

void main(void) {
	// instance seed is used to seed the time offset of this instance
	vec3 instance_seed = vec3(seed, float(gl_InstanceID));
	float t = hash13(instance_seed) + time;

	// particle seed is used to seed the random properties of this particle
	vec4 particle_seed = vec4(instance_seed, floor(t));
	vec4 rand = hash44(particle_seed);

	float state = fract(t);
	float nstate = 1.0 - state;
	float nstate2 = nstate * nstate;
	float nstate4 = nstate2 * nstate2;
	float nstate6 = nstate4 * nstate2;

	float ph = (seed.y - seed.x + t) * (21 + 0.1 * rand.z);
	float d = 0.2 * nstate2;
	vec3 up = v_up + d * vec3(sin(ph + seed.y), 0, cos(ph - seed.x));

	float a = rand.x * tau;
	float sin_a = sin(a);
	float cos_a = cos(a);

	vec3 p = mat3(mat2(cos_a, sin_a, -sin_a, cos_a)) * position;
	p *= mix(min_scale, max_scale, rand.y) * mix(min_shrink, 1, nstate2);
	p += (mix(min_lift, max_lift, rand.z) * lift_scale * state) * up;
	p += mix(min_ofs, max_ofs, rand.w) * (v_forward * sin_a + v_right * cos_a);

	gl_Position = r_projectionMatrix * r_modelViewMatrix * vec4(p, 1.0);
	tex_coord.xy = uv_to_region(sprite_tex_region, texCoordRawIn);
	tex_coord.zw = texCoordRawIn;

	color.rgb = color_base + color_nstate * nstate + color_nstate2 * nstate2;
	color.rgb *= nstate6;
	color.a = smoothstep(0.1, 0.5, state);
	color.a *= color.a;
	color.a *= color.a;

	float f = 0.075;
	color *= smoothstep(0, f, state) * nstate2;

	color *= tint;
}
