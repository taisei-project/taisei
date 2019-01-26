#version 330 core

#include "interface/sprite.glslh"

void main(void) {
    vec4 texel = texture(tex, texCoord);
    fragColor.rgb = color.rgb * texel.g - vec3(0.5 * texel.r) + vec3(texel.b);
    fragColor.a = texel.a * color.a;
}
