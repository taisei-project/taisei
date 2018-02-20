#version 330 core

uniform sampler2D tex;

in vec2 texCoord;
out vec4 fragColor;

uniform vec4 R;
uniform vec4 G;
uniform vec4 B;
uniform vec4 A;
uniform vec4 O;

void main(void) {
	vec4 texel = texture2D(tex, texCoord.xy);

	fragColor = (
		R * texel.r +
		G * texel.g +
		B * texel.b +
		A * texel.a
	) + O;
}
