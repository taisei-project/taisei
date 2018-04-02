#version 330 core

in vec2 texCoord;
uniform sampler2D tex;
uniform vec4 color;
out vec4 outColor;

void main(void) {
    outColor = texture(tex, texCoord) * color;
}
