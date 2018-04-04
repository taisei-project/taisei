#version 330 core

#include "lib/util.glslh"
#include "interface/sprite.glslh"

uniform(1) float t;

void main(void) {
	vec2 r = vec2(texCoord.x-0.5,1.0-texCoord.y);
	float f = abs(r.x)-0.4*pow(r.y,0.3);
	vec3 clr = vec3(0.8+0.2*sin(r.x*2.),0.8+0.2*cos(r.y*2.-t*0.1),0.8+0.2*sin((r.x-r.y)*1.4+t*0.2));

	fragColor = vec4(clr,smoothstep(-(f+0.01*sin(r.y*30.-t*0.7)*(1.-1./(t+1.))+0.05),0.05))+vec4(1.)*smoothstep(-f-0.15-0.2/(1.+t*0.1),0.02);
}
