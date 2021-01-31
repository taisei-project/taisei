#version 330 core

#include "interface/standard.glslh"
#include "lib/util.glslh"

VARYING(3) vec3 posRaw;

void main(void) {
	float phi = atan(posRaw.y,posRaw.x);
	float theta = acos(posRaw.z/length(posRaw));

	vec2 texCoord = vec2(0.5+phi/tau, 1-theta/pi);
	
	fragColor = texture(tex, texCoord);
	fragColor.r = linear_to_srgb(fragColor.r);
	fragColor.g = linear_to_srgb(fragColor.g);
	fragColor.b = linear_to_srgb(fragColor.b);
}
