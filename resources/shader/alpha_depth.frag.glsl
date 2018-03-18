#version 330 core

in vec2 texCoord;
in vec2 texCoordRaw;
out vec4 fragColor;

uniform sampler2D tex;

void main(void) {
	vec4 clr = texture2D(tex, texCoord);
	fragColor = vec4(0.1,0.0,0.07,1.0);
	gl_FragDepth = 0.5*gl_FragCoord.z/(clr.a+0.001);
}
