#version 330 core

uniform sampler2D tex;
uniform vec2 origin;
uniform float ratio; // texture h/w
uniform float t;

float smoothstep(float x) {
	return 1.0/(exp(8.*x)+1.0);
}

in vec2 texCoord;
in vec2 texCoordRaw;
out vec4 fragColor;

void main(void) {
	vec2 pos = texCoordRaw;
	vec4 clr = texture(tex, texCoord);
	pos -= origin;
	pos.y *= ratio;
	float r = length(pos);
	float phi = atan(pos.y,pos.x)+t/10.0;
	float rmin = (1.+0.3*sin(t*20.0+10.*phi))*step(0.,t)*t*t*10.0;
	fragColor = mix(clr, vec4(1.0) - vec4(clr.rgb,0.), step(r,rmin+t*(.1+0.1*sin(10.*r))));

	fragColor.a = float(r > rmin);
}
