#version 330 core

#include "lib/fxaa.glslh"
#include "interface/standard.glslh"

UNIFORM(1) sampler2D depth;
VARYING(3) vec4 fxaa_tc;

void main(void) {
    fragColor = vec4(fxaa(tex, fxaa_tc), 1);
    gl_FragDepth = texture(depth, fxaa_tc.xy).r;
}
