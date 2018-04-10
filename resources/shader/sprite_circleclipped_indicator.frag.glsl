#version 330 core

#include "lib/util.glslh"
#include "interface/sprite.glslh"

uniform(1) vec4 back_color;

void main(void) {
    float fill = customParam;
	vec2 tc = texCoord;
	vec4 texel = texture(tex, tc);
	tc = texCoordRaw - vec2(0.5);
	texel *= mix(back_color, color, float(atan(tc.x,-tc.y) < pi*(2.0*fill-1.0)));
	fragColor = texel;
}
