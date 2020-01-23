#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

void main(void) {
	vec3 c = texture(tex, texCoord).rgb;
	c = max(c, 0);
	c = c / (1.0 + c);
	// c *= smoothstep(0.0, 1.0, sqrt(c));

	// c = c * c;
	fragColor = vec4(c, 0);
}
