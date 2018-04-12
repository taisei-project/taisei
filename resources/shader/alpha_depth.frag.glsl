#version 330 core

#include "interface/standard.glslh"

void main(void) {
	vec4 clr = texture(tex, texCoord);
	fragColor = vec4(0.1,0.0,0.07,1.0);
	gl_FragDepth = 0.5*gl_FragCoord.z/(clr.a+0.001);
}
