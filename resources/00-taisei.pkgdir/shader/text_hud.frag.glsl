#version 330 core

#include "lib/render_context.glslh"
#include "lib/sprite_main.frag.glslh"
#include "lib/util.glslh"

void spriteMain(out vec4 fragColor) {
    float gradient = mix(1.0, 0.5, texCoordOverlay.y);
    vec2 tc = texCoord;

    vec3 outlines = texture(tex, tc).rgb;
    vec4 clr = vec4(color.rgb * gradient, color.a);

    vec4 border = vec4(vec3(0), 0.75 * outlines.g * clr.a);
    vec4 fill = clr * outlines.r;
    vec4 highlight = vec4(vec3(0.15 * outlines.b) * clr.a, 0);

    fragColor = alphaCompose(border, alphaCompose(fill, highlight));
    // fragColor = alphaCompose(border, fill);
}
