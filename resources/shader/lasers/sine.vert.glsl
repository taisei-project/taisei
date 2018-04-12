#version 330 core

#include "common.vert.glslh"

vec2 pos_rule(float t) {
    vec2 line_vel = args[0];
    vec2 line_dir = line_vel / length(line_vel);
    vec2 line_normal = vec2(line_dir.y, -line_dir.x);
    float sine_amp = args[1].x;
    float sine_freq = args[2].x;
    float sine_phase = args[3].x;
    vec2 sine_ofs = line_normal * sine_amp * sin(sine_freq * t + sine_phase);
    return origin + t * line_vel + sine_ofs;
}
