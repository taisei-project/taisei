#version 330 core

#include "common.vert.glslh"

vec2 pos_rule(float t) {
    float diff = args[2].x;
    return vec2(
        args[0].x + args[1].x * (20.0 + 4.0 * diff) * sin(t * 0.025 * diff + args[0].x),
        origin.y + sign((args[0] - origin).y) * 0.06 * t * t
    );
}
