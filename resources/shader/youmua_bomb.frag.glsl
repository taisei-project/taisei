#version 330 core

uniform sampler2D tex;

uniform float tbomb;

in vec2 texCoord;
in vec2 texCoordRaw;
out vec4 fragColor;

void main(void) {
	vec2 pos = texCoordRaw;

	float shift = (2.*float(pos.x<0.5)-1.)*0.1*sqrt(tbomb);
	float f = step(-pos.y*pos.y-abs(pos.x-0.5)+tbomb*(tbomb-0.8)*10.,0.);
	vec3 clr0 = texture(tex, texCoord).rgb;
	pos.y = pos.y*(1.0-abs(shift))+float(shift>0.)*shift;
	vec3 clr = texture(tex, texCoord).rgb;

	clr = (1.-f)*clr0 + f*(atan((clr-vec3(+shift,0.01*tbomb,-shift))*(1.+4.*tbomb*(1.-pow(abs(pos.x-0.5),3.))))/0.8);

	fragColor = vec4(clr,1.0);
}
