#version 330 core

uniform sampler2D depth;
uniform sampler2D tex;
uniform float start;
uniform float end;
uniform float exponent;
uniform float sphereness;
uniform vec4 fog_color;

in vec2 texCoord;
in vec2 texCoordRaw;
out vec4 fragColor;

void main(void) {
	vec2 pos = vec2(texCoord);

	float z = pow(texture2D(depth, texCoord).x+sphereness*length(texCoordRaw-vec2(0.5,0.0)), exponent);
	float f = clamp((end - z)/(end-start),0.0,1.0);

	fragColor = f*texture2D(tex, texCoord) + (1.0-f)*fog_color;
}
