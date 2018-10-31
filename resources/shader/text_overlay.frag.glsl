#version 330 core

#include "lib/render_context.glslh"
#include "interface/sprite.glslh"
#include "lib/util.glslh"

void main(void) {
    float gradient = 0.5 + 0.5 * flip_native_to_bottomleft(texCoordOverlay.y);
    vec2 tc = flip_native_to_topleft(texCoord);

    vec3 outlines = texture(tex, flip_topleft_to_native(tc)).rgb;
    vec4 clr = vec4(color.rgb * gradient, color.a);

    vec4 border = vec4(vec3(0), 0.75 * outlines.g * clr.a);
    vec4 fill = clr * outlines.r;
    vec4 highlight = vec4(vec3(0.25 * outlines.b), 0);

    fragColor = alphaCompose(border, alphaCompose(fill, highlight));
}
