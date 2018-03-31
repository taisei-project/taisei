#version 330 core
#define PI 3.1415926535897932384626433832795

in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D tex;
uniform float fill;
uniform vec4 fill_color;
uniform vec4 back_color;

void main(void) {
	vec2 tc = texCoord;
	vec4 texel = texture(tex, tc);
	tc -= vec2(0.5);
	texel *= mix(back_color, fill_color, float(atan(tc.x,-tc.y) < PI*(2.0*fill-1.0)));
	fragColor = texel;
}
