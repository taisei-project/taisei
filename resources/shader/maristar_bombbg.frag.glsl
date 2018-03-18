#version 330 core

uniform float t;

uniform sampler2D tex;
uniform vec2 plrpos;
uniform float decay;

layout(std140) uniform RenderContext {
	mat4 modelViewMatrix;
	mat4 projectionMatrix;
	mat4 textureMatrix;
	vec4 color;
} ctx;

in vec2 texCoord;
in vec2 texCoordRaw;
out vec4 fragColor;

void main(void) {	
	vec2 r = texCoordRaw-plrpos;
	r = vec2(atan(r.y,r.x)/3.1415*4.,length(r)-2.*t*(1.+decay*t));
	r.x+=(1.+r.y)*t*5.*decay;
	vec4 texel = texture2D(tex, r);
	texel.a = 0.7*min(1.,t)*(1.-t*decay)*4.;

	fragColor = texel*ctx.color;
}
