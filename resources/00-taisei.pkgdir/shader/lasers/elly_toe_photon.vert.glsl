#version 330 core

#include "common.vert.glslh"

vec2 pos_rule(float t) {
    return origin+cmul(args[0],vec2(t,args[2].x*sin(t/args[2].x)));
}
