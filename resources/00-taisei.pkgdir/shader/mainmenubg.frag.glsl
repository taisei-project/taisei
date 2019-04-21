#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/standard.glslh"

UNIFORM(1) float R;
UNIFORM(2) vec2 bg_translation;
UNIFORM(3) vec2 center;
UNIFORM(4) sampler2D tex2;

const float bmin = 3*sqrt(3)/2;

// approximate formula for the real thing I pulled out of my sleeve.
// b is the impact parameter (google) all units are in terms of
// Schwarzschild radii
float bending(float b) {
	float x = b - bmin;
	float a = 2.5;
	float c = 1.136;
	float d = 0.405;
	return pi + 2/b - a*log(tanh(c*pow(x,d)));
}

void main(void) {
	float dx = dFdx(texCoord.x);
	float dy = dFdy(texCoord.y);
	vec2 d = texCoordRaw-vec2(0.5+center.x*dx,0.5+center.y*dy);

	d.y /= dy/dx;

	float b = length(d)/R;
	if(b < bmin) {
		discard;
	}

	float theta = angle(d);

	float phi = bending(b);

	vec3 n = vec3(0);
	n.zx = dir(phi);
	n.xy = rot(theta)*n.xy;

	float step = smoothstep(0.2,0.0,n.x)+smoothstep(-0.6,0,n.z);
	float antialiasing = smoothstep(bmin,bmin+0.03,b);

	n.xy += bg_translation;
	
	vec4 tex1c = texture(tex, n.xy+vec2(0.5,0));
	vec4 tex2c = texture(tex2, n.xy+vec2(0.5));



	fragColor =  r_color * mix(tex1c, tex2c, step);
	fragColor.rgb *= antialiasing;
}
