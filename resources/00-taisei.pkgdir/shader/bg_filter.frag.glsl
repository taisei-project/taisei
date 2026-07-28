#version 330 core

#include "lib/render_context.glslh"
#include "interface/standard.glslh"

UNIFORM(1) mat3 filter_matrix;
UNIFORM(4) vec3 filter_offset;

void main(void) {
	fragColor.rgb = filter_matrix * texture(tex, texCoord).rgb + filter_offset;
	fragColor.a = 1.0;
}
