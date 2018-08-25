#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/sprite.glslh"

void main(void) {
	vec4 texel = texture(tex, texCoord);
	float gradient = 0.8 + 0.2 * flip_native_to_bottomleft(texCoordOverlay.y);
	fragColor = color * texel.r * gradient;
	fragColor.rgb *= gradient;
}
