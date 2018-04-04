#version 330 core

#include "interface/tower_light.glslh"

void main(void) {
	vec4 texel = texture(tex, texCoord);

	float light = (1.6+strength)*dot(normal*(vec3(0.1)+texel.xyz), normalize(l));
	light = max(light, 0.0);

	fragColor = vec4(vec3(texel + color*light)*(light+0.6)*1000.0/length(l),1.0);
}
