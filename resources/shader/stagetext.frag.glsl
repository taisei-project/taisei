#version 330 core

#include "lib/render_context.glslh"

uniform sampler2D trans;
uniform sampler2D tex;

uniform vec3 color;
uniform float t;

in vec2 texCoord;
in vec2 texCoordRaw;
out vec4 fragColor;

void main(void) {
	vec2 pos = texCoordRaw;
	vec2 f = pos-vec2(0.5,0.5);
	pos += (f*0.05*sin(10.0*(length(f)+t)))*(1.0-t);

	pos = clamp(pos,0.0,1.0);
	vec4 texel = texture(tex, (ctx.textureMatrix*vec4(pos,0.0,1.0)).xy);

	texel.a *= clamp((texture(trans, pos).r+0.5)*2.5*t-0.5, 0.0, 1.0);
	fragColor = vec4(color,1.0)*texel;
}

