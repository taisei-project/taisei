#version 330 core

#include "lib/defs.glslh"
#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/sprite.glslh"

void main(void) {
    // Enlarge the quad to make some room for effects.
    float scale = 2;
    vec2 pos = vertPos * scale;
    gl_Position = r_projectionMatrix * spriteVMTransform * vec4(pos, 0.0, 1.0);

    // Adjust texture coordinates so that the glyph remains in the center, unaffected by the scaling factor.
    // Some extra code is required in the fragment shader to chop off the unwanted bits of the texture.
    vec2 tc = vertTexCoord * scale - vec2(0.5 * (scale - 1));

    // Pass tc not mapped to the texture region, because we want to do per-fragment transformations on it.
    texCoord = tc;

    // Pass the normalized texture region, so that we can map texCoord to it later in the fragment shader.
    texRegion = spriteTexRegion;

    // Global overlay coordinates for this primitive.
    texCoordOverlay = (spriteTexTransform * vec4(tc, 0.0, 1.0)).xy;

    // Fragment shader needs to know the sprite dimensions so that it can denormalize texCoord for processing.
    dimensions = spriteDimensions;

    // Arbitrary custom parameter provided by the application. You can use it to pass e.g. times/frames.
    customParam = spriteCustomParam;

    // Should be obvious.
    color = spriteRGBA;
}
