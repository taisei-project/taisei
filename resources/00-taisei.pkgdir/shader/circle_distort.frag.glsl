#version 330 core

#include "lib/util.glslh"
#include "lib/render_context.glslh"
#include "interface/standard.glslh"

UNIFORM(1) vec2 viewport;
UNIFORM(2) vec3 distortOriginRadius;
UNIFORM(3) float time;

void main(void) {
	vec2 uv = texCoord;
	vec2 uv_orig = uv;
	vec2 distort_origin = distortOriginRadius.xy;
	float distort_radius = distortOriginRadius.z;
	float dst = distort_radius / length(uv * viewport - distort_origin);

	distort_origin /= viewport;
	uv -= distort_origin;

	float mask = min(1, sqrt(dst));

	float m = log(1.0 + dst * dst);
	float a = 0.01 * smoothstep(0.1, 1.0, 1 - m);
	float f = 54;

	uv.x += a * sin(uv.y * f * 1.321 + time * 4.345);
	uv.y += a * sin(uv.x * f * 1.458 - time * 6.431);
	a *= 0.5;
	f *= 3.0;
	uv.x += a * sin(uv.y * (f * 1.531) - time * 54.961);
	uv.y += a * sin(uv.x * (f * 1.931) + time * 31.345);

	uv = pow(abs(uv), vec2(1.1 - sqrt(dst) * 0.05)) * sign(uv);
	uv += distort_origin;

	vec4 smpl = texture(tex, uv);
	fragColor = smpl * smpl * mask;

}
