#version 330 core

#include "common.vert.glslh"

vec2 pos_rule(float t) {
    vec2 p = vec2(t, args[2].x*t*0.02*sin(0.1*t+args[2].y));
    return origin + vec2(args[0].x*p.x - args[0].y*p.y, args[0].x*p.y + args[0].y*p.x);
}
