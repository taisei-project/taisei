#version 330 core

#include "lib/render_context.glslh"

uniform sampler2D tex;
uniform vec2 blur_orig; // center
uniform vec2 fix_orig;
uniform float blur_rad;  // radius of zoom effect
uniform float rad;
uniform float ratio; // texture h/w
uniform vec4 color;

in vec2 texCoord;
in vec2 texCoordRaw;
out vec4 fragColor;

void main(void) {
	vec2 pos = texCoordRaw;
	pos -= blur_orig;

	vec2 pos1 = pos;
	pos1.y *= ratio;

	pos *= min(length(pos1)/blur_rad,1.0);
	pos += blur_orig;
	pos = clamp(pos, 0.005, 0.995);
	pos = (ctx.textureMatrix * vec4(pos,0.0,1.0)).xy;

	fragColor = texture2D(tex, pos);

	pos1 = texCoordRaw - fix_orig;
	pos1.y *= ratio;

	fragColor *= pow(color,vec4(3.0*max(0.0,rad - length(pos1))));
}
