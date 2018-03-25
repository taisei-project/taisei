#version 330 core

#include "render_context.glslh"

uniform sampler2D tex;

in vec2 texCoord;
in vec4 color;

out vec4 fragColor;

void main(void) {
    vec4 texel = texture(tex, texCoord);
    fragColor.rgb = mix(color.rgb, vec3(1.0), texel.b);
    fragColor.a = texel.a * color.a;
}
