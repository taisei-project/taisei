#version 330 core

#include "common.vert.glslh"

vec2 pos_rule(float t) {
    // XXX: should turn speed be in rad/sec or rad/frame? currently rad/sec.
    float turn_speed = args[0].x / 60;
    float time_ofs = args[0].y;
    float radius = args[1].x;
    return origin + radius * dir((t + time_ofs) * turn_speed);
}
