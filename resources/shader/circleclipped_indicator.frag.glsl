#version 330 core

#include "lib/util.glslh"
#include "interface/standard.glslh"

uniform(1) float fill;
uniform(2) vec4 fill_color;
uniform(3) vec4 back_color;

void main(void) {
	vec2 tc = texCoord;
	vec4 texel = texture(tex, tc);
	tc -= vec2(0.5);
	texel *= mix(back_color, fill_color, float(atan(tc.x,-tc.y) < pi*(2.0*fill-1.0)));
	fragColor = texel;
}
