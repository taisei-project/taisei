#version 330 core

#include "interface/standard.glslh"
#include "lib/util.glslh"

UNIFORM(1) sampler2D shotlayer;
UNIFORM(2) sampler2D flowlayer;
UNIFORM(3) float time;

float f(float x) {
	x = smoothstep(0.0, 1.0, x * x);
	return x;
}

vec4 f(vec4 v) {
	return vec4(f(v.x), f(v.y), f(v.z), f(v.w));
}

vec4 colormap(vec4 c) {
	float a = f(c.a);
	vec3 hsv = rgb2hsv(c.rgb);
	hsv.y *= (1 - a * a * a * a);
	hsv.z += (a * a * a) * (1 - hsv.z);
	return clamp(vec4(hsv2rgb(hsv), a * 0.5), 0.0, 1.0);
}

void main(void) {
	vec2 uv = texCoord;
	vec2 gradX = dFdx(uv);
	vec2 gradY = dFdy(uv);
	vec4 c = textureGrad(shotlayer, uv, gradX, gradY);
	vec4 bg = textureGrad(tex, uv, gradX, gradY);

	if(c.a <= 0) {
		fragColor = bg;

		// uncomment for culling visualization
		// fragColor = vec4(1, 0, 0, 1);

		return;
	}

	c = colormap(c);

	vec4 m = vec4(vec3(0), 1);
	float t = (time + c.a * 0.2) * 0.1 + 92.123431 + fwidth(c.a);

	m.r += textureGrad(flowlayer, uv + 0.0524 * vec2(3.123*sin(t*1.632), -t*36.345), gradX, gradY).r;
	m.g += textureGrad(flowlayer, uv + 0.0513 * vec2(3.312*sin(t*1.213), -t*31.314), gradX, gradY).g;
	m.b += textureGrad(flowlayer, uv + 0.0551 * vec2(3.341*sin(t*1.623), -t*35.642), gradX, gradY).b;

	m.r += textureGrad(flowlayer, uv + 0.0531 * vec2(t*13.624, -t*49.851), gradX, gradY).g;
	m.g += textureGrad(flowlayer, uv + 0.0501 * vec2(t*12.314, -t*42.123), gradX, gradY).b;
	m.b += textureGrad(flowlayer, uv + 0.0593 * vec2(t*11.324, -t*44.681), gradX, gradY).r;

	m.r += textureGrad(flowlayer, uv + 0.0543 * vec2(-t*12.931, -t*24.341), gradX, gradY).b;
	m.g += textureGrad(flowlayer, uv + 0.0534 * vec2(-t*11.123, -t*23.934), gradX, gradY).r;
	m.b += textureGrad(flowlayer, uv + 0.0584 * vec2(-t*13.631, -t*25.341), gradX, gradY).g;

	m.rgb = hueShift(clamp(m.rgb / 3, 0, 1), time * 0.1 + uv.y);

	fragColor = alphaCompose(bg, c * m);
}
