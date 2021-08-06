#version 330 core

#include "../lib/render_context.glslh"
#include "../lib/util.glslh"
#include "../interface/standard.glslh"

#define SDF_RANGE 16.0

UNIFORM(1) float laser_width;

void main(void) {
	vec2 s = texture(tex, texCoord).xy;
	float d = -s.x;

	if(d < -4) {
		discard;
	}

	float t = s.y;

	float bFactor = smoothstep( 0, 6, d      );
	float hFactor = smoothstep( 2, 7, d + 0.8);
	float gFactor = smoothstep(-min(4, laser_width), 8, d + 2);

	hFactor = hFactor * hFactor;
	gFactor = gFactor * gFactor;

	vec3 color = (bFactor + gFactor) * r_color.rgb + hFactor;
	// color *= r_color.a;

	// test using the time dimension
	// color = hueShift(color, t * 0.001);

	fragColor = vec4(color, 0);
}
