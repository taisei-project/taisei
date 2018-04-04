#version 330 core

#include "interface/sprite.glslh"

void main(void) {
    vec4 texel = texture(tex, texCoord);
    fragColor = vec4(0.0);
    fragColor.g += 0.75 * texel.r;
    fragColor.rgb += color.rgb * texel.g;
    fragColor.rgb += vec3(texel.b);
    fragColor.a = texel.a * color.a;
}
