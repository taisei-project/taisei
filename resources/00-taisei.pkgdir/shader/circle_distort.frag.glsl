#version 330 core

#include "lib/util.glslh"
#include "lib/render_context.glslh"
#include "interface/standard.glslh"

UNIFORM(1) vec2 viewport;
UNIFORM(2) vec3 distortOriginRadius;

void main(void) {
	vec2 uv = texCoord;
	vec2 uv_orig = uv;
	vec2 distort_origin = distortOriginRadius.xy;
	float dst = distortOriginRadius.z / length(uv * viewport - distort_origin);

	distort_origin /= viewport;
	uv -= distort_origin;
	uv = uv * uv * sign(uv);
	uv *= rot(-dst);
	uv += distort_origin;

	float m = log(1.0 + dst * dst);
	uv = mix(uv_orig, uv, m);

	fragColor = r_color * texture(tex, uv) * min(sqrt(m), 1.0);
}
