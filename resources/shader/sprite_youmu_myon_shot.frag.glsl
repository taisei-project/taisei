#version 330 core

#include "lib/render_context.glslh"

uniform sampler2D tex;

in vec2 texCoord;
in vec4 color;

out vec4 fragColor;

void main(void) {
    vec4 texel = texture(tex, texCoord);

    fragColor = vec4(0.0);
    fragColor.rgb += vec3(texel.r);
    fragColor.rgb += texel.b * vec3(color.r*color.r, color.g*color.g, color.b*color.b);
    fragColor.a = texel.a * color.a;
}
