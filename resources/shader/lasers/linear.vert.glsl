#version 330 core

#include "common.vert.glslh"

vec2 pos_rule(float t) {
    return origin + args[0]*t;
}
