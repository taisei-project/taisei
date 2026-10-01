#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/standard.glslh"

UNIFORM(1) sampler2D noise;
UNIFORM(2) float time;
UNIFORM(3) float sdf_range;

void main(void) {
	vec2 dist = texture(tex, texCoord).rg;
	// dist.x = dist.y;

	if(dist.x >= sdf_range * 0.9) {
		// fragColor = vec4(vec3(0, 1, 0), 1); return;
		discard;
	}

	vec2 uv = texCoord;
	float n0 = texture(noise, 1.00 * (uv + time * vec2( 0.04,  0.06312))).r;
	float n1 = texture(noise, 0.75 * (uv + time * vec2(-0.04,  0.04310))).r;
	float n2 = texture(noise, 1.25 * (uv + time * vec2( 0.05, -0.07310))).r;
	float n3 = texture(noise, 1.50 * (uv + time * vec2(-0.05, -0.04531))).r;

	float n = max(n0, n2) * max(n1, n3);
	float k = mix(max(n0, n1) * max(n2, n3), n, n);
	float J = k / (max(1, dist.x - sdf_range * 0.5) * (n + 0.001));
	float displacement = 4 * J + n2 - n3 + 5 * sin(2 * pi * n0);

	dist.x -= displacement;

	float border = min(1, 1 / max(0.0001, dist.x));
	border *= 1 - (dist.x / sdf_range);

	if(border < 1/255.0) {
		// fragColor = vec4(vec3(1, 0.5, 0), 1); return;
		discard;
	}

	float inner = smoothstep(1, -0.25, 0.25 * dist.x);
	float core =
		 smoothstep(sdf_range * 0.1, -sdf_range * 0.01 - displacement, dist.y - k*20)
		-smoothstep(0,               -sdf_range * 0.50 + displacement, dist.y + n*20);

	vec4 c = vec4(2 * border, 0, 0, 0);
	c = alphaCompose(c, inner * vec4(n, 0, n-k, 1));
	c = alphaCompose(c, vec4(core, k*core, 5*(n-k)*core*core*core*core, 0));
	// c.a *= (1 - c.r) * (1 - c.r);
	c.a *= sqrt(c.r);

	// c = vec4(core);

	fragColor = c;
}
