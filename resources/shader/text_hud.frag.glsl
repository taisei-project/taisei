#version 330 core

#include "lib/render_context.glslh"
#include "interface/sprite.glslh"

void main(void) {
	vec4 texel = texture(tex, texCoord);
	float gradient = mix(1.0, 0.8, texCoordOverlay.y);
	fragColor = color * texel.r * gradient;
	fragColor.rgb *= gradient;
}
