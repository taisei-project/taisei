#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/standard.glslh"

UNIFORM(1) vec4 snap_angles_sincos;
UNIFORM(2) vec4 joy_pointers;
UNIFORM(3) vec2 deadzones;

float sdCircle(vec2 p, float r) {
	return length(p) - r;
}

float sdRoundedX(vec2 p, float w, float r) {
	p = abs(p);
	return length(p - min(p.x + p.y, w) * 0.5) - r;
}

float sdPieDual(vec2 p, vec2 c, float r) {
	p = abs(p);
	float l = length(p) - r;
	float m = length(p - c * clamp(dot(p, c), 0.0, r));
	return max(l, m * sign(c.y * p.x - c.x * p.y));
}

vec2 sincos(float a) {
	return vec2(sin(a), cos(a));
}

float zones(vec2 uv, vec2 sc) {
	return 0.5 - min(
		sdPieDual(uv, sc, 1.0),
		sdPieDual(uv.yx, sc, 1.0));
}

vec4 joystickPointer(vec2 p, float d, vec4 color) {
	float base = 1.0 - sdCircle(p, -0.4);

	float outer = smoothstep(0.48, 0.52, base) - smoothstep(0.513-d, 0.513+d, base);
	vec4 c = mix(vec4(color.a), color, outer) * outer;

	float inner = smoothstep(0.51, 0.53, base);
	float innerGrad = smoothstep(0.52, 0.55, base);
	c = alphaCompose(c, vec4(color.rgb * mix(1.0, 0.75, innerGrad), color.a) * inner);

	return c * c.a;
}

void main(void) {
	vec2 uv = 2.22 * (flip_native_to_topleft(texCoordRaw) - 0.5);
	vec2 snapCardinalsSinCos = snap_angles_sincos.xy;
	vec2 snapDiagonalsSinCos = snap_angles_sincos.zw;

	vec4 c = vec4(0);

	float d = fwidth(uv.x);
	float circleBase = 1.0 - sdCircle(uv, 0.5);
	float circle = smoothstep(0.5 - d, 0.5 + d, circleBase);
	c = alphaCompose(c, vec4(0.25 * circle));

	float deadzone = smoothstep(0.5 - 0.01, 0.5 + 0.01, circleBase - (1 - deadzones.x));
	float snapMask = circle * (1 - deadzone);
	deadzone *= 1.0 - smoothstep(0.25, 0.7, circleBase - (1 - deadzones.x + 0.2));

	float maxzone_val = 1 - max(deadzones.x, deadzones.y);
	float maxzone = smoothstep(0.5 - maxzone_val, 0.5 + d, circleBase - maxzone_val);
	maxzone -= smoothstep(0.52 - d, 0.52 + d, circleBase - maxzone_val);

	float cross1 = sdRoundedX(uv * rot(pi/4), 2.0, -0.49);
	float cross2 = sdRoundedX(uv, 2.0, -0.49);
	float delimiters = smoothstep(0.48, 0.6, circle * (1.0 - min(cross1, cross2)));

	float cardinals = zones(uv, snapCardinalsSinCos);
	float diagonals = zones(uv * rot(pi/4), snapDiagonalsSinCos);

	c = alphaCompose(c, vec4(0.5 * maxzone));
	c = alphaCompose(c, vec4(1, 0.3, 0.3, 1) * 0.5 * snapMask * smoothstep(0.5-d, 0.5+d, cardinals));
	c = alphaCompose(c, vec4(0.3, 0.3, 1, 1) * 0.5 * snapMask * smoothstep(0.5-d, 0.5+d, diagonals));
	c = alphaCompose(c, vec4(vec3(0), 0.7 * deadzone));
	c = alphaCompose(c, joystickPointer(uv - joy_pointers.xy, d, vec4(1, 0.9, 0.2, 1)));
	c = alphaCompose(c, joystickPointer(uv - joy_pointers.zw, d, vec4(0.6)));
	c = alphaCompose(c, vec4(0, 0, 0, delimiters));

	if(all(lessThan(c, vec4(1.0 / 255.0)))) {
		discard;
	}

	fragColor = c * r_color;
}
