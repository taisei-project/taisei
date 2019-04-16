#version 330 core

#include "lib/sprite_main.frag.glslh"

void spriteMain(out vec4 fragColor) {
    vec4 texel = texture(tex, texCoord);
    fragColor.rgb = mix(color.rgb, vec3(0.0), texel.b) * texel.a;
    fragColor.rgb += customParams.rgb * texel.r * texel.a * customParams.a;
    // fragColor.rgb = mix(fragColor.rgb, customParams.rgb, customParams.a * texel.r * texel.a);
    fragColor.a = texel.a * color.a;
}
