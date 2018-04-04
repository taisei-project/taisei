#version 330 core

#include "interface/standard.glslh"

in(3) vec3 posModelView;

void main(void) {
	vec4 texel = texture(tex, texCoord);
	float f = min(1.0, length(posModelView)/3000.0);
	fragColor = mix(vec4(1.0), texel, 1.0-f);
}
