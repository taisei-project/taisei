#version 330 core

uniform sampler2D tex;
uniform vec4 color;

in vec2 texCoord;
in vec2 texCoordRaw;
out vec4 fragColor;

void main(void) {
	vec4 texel = texture(tex, texCoord);
	vec4 mixclr = 1.0-texel;
	mixclr.a = texel.a;
	fragColor = mixclr;
}
