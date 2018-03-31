#version 330 core

#include "common.vert.glslh"

vec2 pos_rule(float t) {
    vec2 v0 = args[0];
    vec2 v1 = args[1];
    float begin = args[2].x;
    float end = args[2].y;
    float a = clamp((t - begin) / (end - begin), 0.0, 1.0);
    a = 1.0 - (0.5 + 0.5 * cos(a * pi));
    a = 1.0 - pow(1.0 - a, 2);
    return origin + mix(v0, v1, a) * t;
}
