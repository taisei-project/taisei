#version 330 core

#include "lib/defs.glslh"
#include "lib/util.glslh"
#include "interface/standard.glslh"

uniform(1) vec3 color;
uniform(2) float mixfactor;

void main(void) {
	vec3 rgb = texture(tex, texCoord).rgb;

	float	lum1	= lum(rgb);
	float	lum2	= lum(color);
	vec3	white1	= vec3(min3(rgb));
	vec3	white2	= vec3(min3(color));
	vec3	newclr	= white1 + (color - white2) * (lum2/lum1);

	fragColor	= mix(vec4(rgb, 1.0), vec4(pow(newclr, vec3(1.3)), 1.0), mixfactor);
}
