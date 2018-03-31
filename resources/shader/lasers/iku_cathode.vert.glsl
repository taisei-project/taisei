#version 330 core

#include "common.vert.glslh"

vec2 pos_rule(float t) {
    return origin + cmul(t * args[0], dir(args[1].y * t));
}
