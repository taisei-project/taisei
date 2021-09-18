#version 330 core

#include "lib/defs.glslh"
#include "interface/particle_emitter.glslh"

void main(void) {
	vec4 c = color * texture(sprite_tex, tex_coord.xy);

	if(all(lessThan(c, vec4(1.0/256.0)))) {
		discard;
	}

	fragColor = c;
}
