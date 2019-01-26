#version 330 core

#include "common.vert.glslh"

vec2 pos_rule(float t) {
    float s = (args[2].x * t + args[3].x);
    return origin + dir(angle(args[0]) + args[1].x * sin(s)) * t * length(args[0]);
}
