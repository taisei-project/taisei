#version 330

uniform sampler2D tex;
uniform vec2 origin;
uniform float ratio; // texture h/w
uniform float t;

layout(std140) uniform RenderContext {
	mat4 modelViewMatrix;
	mat4 projectionMatrix;
	mat4 textureMatrix;
	vec4 color;
} ctx;

in vec2 texCoordRaw;

float smoothstep(float x) {
	return 1.0/(exp(8.*x)+1.0);
}

void main(void) {
	vec2 pos = texCoordRaw;
	vec4 clr = texture2D(tex, (ctx.textureMatrix*vec4(pos,0.0,1.0)).xy);
	pos -= origin;
	pos.y *= ratio;
	float r = length(pos);
	float phi = atan(pos.y,pos.x)+t/10.0;
	float rmin = (1.+smoothstep(t-.5)*sin(t*40.0+10.*phi))*step(0.,t)*t*1.5;
	gl_FragColor = mix(clr, vec4(1.0) - vec4(clr.rgb,0.), step(rmin-.1*sqrt(r),r));

	gl_FragColor.a = float(r < rmin);
}
