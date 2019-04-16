#version 330 core

#include "lib/sprite_main.frag.glslh"

void spriteMain(out vec4 fragColor) {
    vec4 texel = texture(tex, texCoord);

    fragColor = vec4(0.0);
    fragColor.rgb += vec3(texel.r);
    fragColor.rgb += texel.b * vec3(color.r*color.r, color.g*color.g, color.b*color.b);
    fragColor.a = texel.a * color.a;
    fragColor *= (1 - customParams.r);
}
