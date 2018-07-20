#version 330 core

#include "interface/sprite.glslh"

void main(void) {
    vec4 texel = texture(tex, texCoord);

    fragColor = vec4(0.0);
    fragColor.rgb += vec3(texel.r);
    fragColor.rgb += texel.b * vec3(color.r*color.r, color.g*color.g, color.b*color.b);
    fragColor.a = texel.a * color.a;
    fragColor *= (1 - customParam);
}
