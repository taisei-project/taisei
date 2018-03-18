#version 330 core

uniform sampler2D tex;

in vec3 posRaw;
out vec4 fragColor;

void main(void) {
	vec4 clr = clamp(vec4(0.0)+pow(1.0-posRaw.z*posRaw.z,5.)*vec4(0.1,0.5,0.9,1.0)+exp(-posRaw.z*10.0)*vec4(1.0),0.,1.);

	clr = mix(clr,vec4(0.2,0.4,0.6,1.0),step(0.,-posRaw.z)*exp(-10.*sqrt(1.+1.*length(posRaw.xy))));
	fragColor = clr;
}
