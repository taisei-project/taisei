#version 330 core

uniform sampler2D tex;

in vec3 posRaw;
in vec2 texCoord;
out vec4 fragColor;

void main(void) {
	vec4 texel = texture2D(tex, texCoord);
	float f = min(1.0, length(posRaw)/3000.0);
	gl_FragColor = mix(vec4(1.0), texel, 1.0-f);
}
