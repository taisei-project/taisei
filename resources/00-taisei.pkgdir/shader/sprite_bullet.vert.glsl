#version 330 core

#include "lib/defs.glslh"
#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/sprite.glslh"

void main(void) {
    gl_Position = r_projectionMatrix * spriteVMTransform * vec4(vertPos, 0.0, 1.0);
    texCoordRaw = vertTexCoord;
    texCoord = uv_to_region(spriteTexRegion, vertTexCoord);
    texRegion = spriteTexRegion;
    customParams = spriteCustomVec;
    color = spriteRGBA;
}
