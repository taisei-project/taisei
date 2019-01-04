#version 330 core

#include "lib/render_context.glslh"
#include "interface/healthbar.glslh"

void main(void) {
    gl_Position = r_projectionMatrix * r_modelViewMatrix * vec4(position, 1.0);
    texCoord = (r_textureMatrix * vec4(texCoordRawIn, 0.0, 1.0)).xy;

    fillNormAngles = vec4(
        0.5 - fill.x * 0.5, // min fill
        0.5 + fill.x * 0.5, // max fill
        0.5 - fill.y * 0.5, // min alt
        0.5 + fill.y * 0.5  // max alt
    );
}
