#version 330 core

#include "lib/render_context.glslh"
#include "interface/sprite.glslh"

void main(void) {
	vec4 texel = texture(tex, texCoord);
	fragColor = vec4(color.rgb, color.a * texel.r) * mix(1.0, 0.8, texCoordOverlay.y);
}
