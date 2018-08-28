#version 330 core

#include "lib/util.glslh"
#include "interface/sprite.glslh"

UNIFORM(1) vec4 back_color;

void main(void) {
	float fill = customParams.r;
	vec4 texel = texture(tex, texCoord);
	vec2 tc = flip_native_to_bottomleft(texCoordRaw) - vec2(0.5);
	texel *= mix(back_color, color, float(atan(tc.x, tc.y) < pi*(2.0*fill-1.0)));
	fragColor = texel;
}
