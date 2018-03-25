#version 330 core

#include "render_context.glslh"

uniform sampler2D tex;

in vec2 texCoord;
in vec4 color;

out vec4 fragColor;

void main(void) {
    vec4 texel = texture(tex, texCoord);
    fragColor = vec4(0.0);
    fragColor.g += 0.75 * texel.r;
    fragColor.rgb += color.rgb * texel.g;
    fragColor.rgb += vec3(texel.b);
    fragColor.a = texel.a * color.a;
}
