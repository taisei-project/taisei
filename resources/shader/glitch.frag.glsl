#version 330 core

#include "interface/standard.glslh"

uniform(1) int frames;
uniform(2) float strength;

float f(float x) {
    return cos(floor(20.0 * x) - cos((30.0 * x)) * sin(floor(4.0 * x)));
}

void main(void) {
    vec2 p = texCoord;

    float t = float(frames) / 54.3;
    float s = clamp(pow(cos(f(t/3.1 - p.y + 0.15 * f(p.x+t)) + f(t) + (floor(128.0 * sin(p.x * 0.01 + 0.3 * t)) / 128.0) * 53.2), 100.0), 0.0, 1.0);
    float g = f(f(p.y + (float(frames) / 33.2)) + floor(float(frames) / 30.2));

    p.x -= strength * 0.5 * g * s;
    p = clamp(p, 0.0, 1.0);

    fragColor = texture(tex, p);
}
