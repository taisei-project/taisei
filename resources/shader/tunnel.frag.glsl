#version 330 core

uniform sampler2D tex;
uniform vec3 color;
uniform float mixfactor;

in vec2 texCoord;
in vec2 texCoordRaw;
out vec4 fragColor;

float min(vec3 c) {
	float m = c.r;
	if(c.g < m)
		m = c.g;
	if(c.b < m)
		m = c.b;
	return m;
}

float lum(vec3 c) {
	return 0.216 * c.r + 0.7152 * c.g + 0.0722 * c.b;
}

void main(void) {
	vec3 rgb = texture(tex, texCoord).rgb;

	float	lum1	= lum(rgb);
	float	lum2	= lum(color);
	vec3	white1	= vec3(min(rgb));
	vec3	white2	= vec3(min(color));
	vec3	newclr	= white1 + (color - white2) * (lum2/lum1);

	fragColor	= mix(vec4(rgb, 1.0), vec4(pow(newclr, vec3(1.3)), 1.0), mixfactor);
}
