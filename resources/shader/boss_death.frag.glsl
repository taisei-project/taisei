
#version 330 core

#include "interface/standard.glslh"
#include "lib/util.glslh"

UNIFORM(1) vec2 viewport;
UNIFORM(2) vec2 origin;
UNIFORM(3) vec2 clear_origin;
UNIFORM(4) int frames;
UNIFORM(5) float progress;
UNIFORM(6) sampler2D noise_tex;

bool in_circle(vec2 pos, vec2 ofs, float a, vec2 origin, float radius) {
    mat2 m = mat2(cos(a), -sin(a), sin(a), cos(a));
    return length(pos + m * ofs - origin) < radius;
}

float f(float x, float s) {
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
    float s = sqrt(pow(viewport.x, 2) + pow(viewport.y, 2));
    float o = 32 * (1 + progress);
    bvec3 mask = bvec3(false);
    float a = frames / 30.0;

    for(int i = 0; i < 3; ++i) {
        float q = 1.0 + 0.01 * i * progress;

        mask[i] = mask[i] != in_circle(p,  vec2(0, 0),  1*a,       origin, s * f(pow(progress * 1.5, 1.5), 1.0*q));
        mask[i] = mask[i] != in_circle(p,  vec2(o, 0),  1*a,       origin, s * f(pow(progress * 1.5, 2.0), 1.0*q));
        mask[i] = mask[i] != in_circle(p,  vec2(0, o),  1*a,       origin, s * f(pow(progress * 1.5, 2.0), 1.0*q));
        mask[i] = mask[i] != in_circle(p, -vec2(o, 0),  1*a,       origin, s * f(pow(progress * 1.5, 2.0), 1.0*q));
        mask[i] = mask[i] != in_circle(p, -vec2(0, o),  1*a,       origin, s * f(pow(progress * 1.5, 2.0), 1.0*q));

        mask[i] = mask[i] != in_circle(p,  vec2(0, 0), -2*a,       origin, s * f(pow(progress * 1.5, 1.5/1.5), 1.0*q));
        mask[i] = mask[i] != in_circle(p,  vec2(o, 0), -2*a,       origin, s * f(pow(progress * 1.5, 1.5/2.0), 1.0*q));
        mask[i] = mask[i] != in_circle(p,  vec2(0, o), -2*a,       origin, s * f(pow(progress * 1.5, 1.5/2.0), 1.0*q));
        mask[i] = mask[i] != in_circle(p, -vec2(o, 0), -2*a,       origin, s * f(pow(progress * 1.5, 1.5/2.0), 1.0*q));
        mask[i] = mask[i] != in_circle(p, -vec2(0, o), -2*a,       origin, s * f(pow(progress * 1.5, 1.5/2.0), 1.0*q));
    }

    fragColor = mix(pclr, nclr, vec4(mask, 0));
}
