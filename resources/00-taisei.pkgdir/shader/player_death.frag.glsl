
#version 330 core

#include "interface/standard.glslh"
#include "lib/util.glslh"

UNIFORM(1) vec2 viewport;
UNIFORM(2) vec2 origin;
UNIFORM(3) vec2 clear_origin;
UNIFORM(4) int frames;
UNIFORM(5) float progress;
UNIFORM(6) sampler2D noise_tex;
UNIFORM(7) float size;

vec3 xor(vec3 a, vec3 b) {
    return abs(a - b);
}

vec3 in_circle(vec2 pos, vec2 ofs, float a, vec2 origin, vec3 radius) {
    return step(vec3(length(pos + rot(a) * ofs - origin)), radius);
}

vec3 f(float x, vec3 s) {
    return s * x - s + 1;
}

float invert(float x) {
    return (0.5 + 0.5 * cos(pi * x)) * (1 - x);
}

vec3 invert(vec3 v) {
    return vec3(
        invert(v.r),
        invert(v.g),
        invert(v.b)
    );
}

void main(void) {
    vec3 colormod = texture(noise_tex, vec2(texCoord.x + frames/43.0 * sin(texCoord.y * 42), texCoord.y - frames/30.0)).rgb;
    vec4 pclr = texture(tex, texCoord);
    vec4 nclr = pclr;
    nclr.rgb = colormod * vec3(lum(invert(nclr.rgb)));
    nclr.rgb = max(colormod * vec3(0.1, 0.1, 0.3), nclr.rgb);

    vec2 p = texCoord * viewport;
    float o = 32 * (1 + progress);
    vec3 mask = vec3(0);
    float a = frames / 30.0;

    vec3 i = vec3(1, 2, 3);
    vec3 q = 1.0 + 0.01 * i * progress;

    mask = xor(mask, in_circle(p,  vec2(0, 0),  1*a,       origin, size * f(pow(progress * 1.5, 1.5), 1.0*q)));
    mask = xor(mask, in_circle(p,  vec2(o, 0),  1*a,       origin, size * f(pow(progress * 1.5, 2.0), 1.0*q)));
    mask = xor(mask, in_circle(p,  vec2(0, o),  1*a,       origin, size * f(pow(progress * 1.5, 2.0), 1.0*q)));
    mask = xor(mask, in_circle(p, -vec2(o, 0),  1*a,       origin, size * f(pow(progress * 1.5, 2.0), 1.0*q)));
    mask = xor(mask, in_circle(p, -vec2(0, o),  1*a,       origin, size * f(pow(progress * 1.5, 2.0), 1.0*q)));

    mask = xor(mask, in_circle(p,  vec2(0, 0), -2*a, clear_origin, size * (1 - pow(vec3(2 - 2 * progress), 4+0.10*i))));
    mask = xor(mask, in_circle(p,  vec2(o, 0), -2*a, clear_origin, size * (1 - pow(vec3(2 - 2 * progress), 2+0.05*i))));
    mask = xor(mask, in_circle(p, -vec2(o, 0), -2*a, clear_origin, size * (1 - pow(vec3(2 - 2 * progress), 2+0.05*i))));
    mask = xor(mask, in_circle(p,  vec2(0, o), -2*a, clear_origin, size * (1 - pow(vec3(2 - 2 * progress), 2+0.05*i))));
    mask = xor(mask, in_circle(p, -vec2(0, o), -2*a, clear_origin, size * (1 - pow(vec3(2 - 2 * progress), 2+0.05*i))));

    fragColor = mix(pclr, nclr, vec4(mask, 0));
}
