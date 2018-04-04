#version 330 core

#include "interface/sprite.glslh"

void main(void) {
    vec4 texel = texture(tex, texCoord);
    fragColor.rgb = mix(color.rgb, vec3(1.0), texel.b);
    fragColor.a = texel.a * color.a;
}
