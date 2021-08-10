#version 330 core

#include "../lib/render_context.glslh"
#include "../lib/util.glslh"
#include "../interface/laser_pass2.glslh"

void main(void) {
	vec2 s = texture(tex, texCoord).xy;

	if(s.x >= 4) {
		discard;
	}

	float d = -s.x;

	vec3 laser_color = color_width.rgb;
	float laser_width = color_width.a;

	float t = s.y;

	float bFactor = smoothstep( 0, 6, d      );
	float hFactor = smoothstep( 2, 7, d + 0.8);
	float gFactor = smoothstep(-min(4, laser_width), 8, d + 2);

	hFactor = hFactor * hFactor;
	gFactor = gFactor * gFactor;

	vec3 color = (bFactor + gFactor) * laser_color + hFactor;
	// color *= laser_color.a;

	// test using the time dimension
	// color = hueShift(color, t * 0.001);

	fragColor = vec4(color, 0);
}
