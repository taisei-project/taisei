#version 330 core

#include "lib/sprite_main.frag.glslh"

void spriteMain(out vec4 fragColor) {
    vec3 outlines = texture(tex, texCoord).rgb;

    vec4 highlight = color;
    vec4 fill      = vec4(color.rgb * 0.9, color.a);
    vec4 border    = vec4(color.rgb * 0.3, color.a);

    fragColor = outlines.g * mix(border, mix(fill, highlight, outlines.b), outlines.r);
}
