#version 330 core

#include "lib/util.glslh"
#include "interface/spellcard.glslh"

void main(void) {
	float n = 10; // rough number of rings of text

	vec2 pos = (vec2(flip_native_to_topleft(texCoordRaw)) - origin * vec2(ratio, 1.0)) * n;
	pos.x /= ratio;

	// Slightly distorted mapping to polar coordinates. I wrote this
	// shortly after hearing a general relativity lecture...

	pos = vec2(atan(pos.x, pos.y), length(pos) + n * t * 0.1);

	// The idea is (apart from the aspect factors) to map the
	// (phi, angle) coordinates to something that loops several
	// times and contains margins.
	// This is achieved using all those mods.
	//
	// Simplified example:
	// f(x) = x for 0 < x < 2
	// mod(f(x),1) then goes from 0 to 1 and from 0 to 1 again.
	//
	// If you use it as a texcoord you get two copies of the texture.
	//
	// The complicated example I don’t understand either:

	pos.x *= h/w;
	pos.x += t * 0.5 * float(2 * int(mod(pos.y, 2)) - 1);
	pos = mod(pos, vec2(1 + 0.01 / w, 1));
	pos = flip_topleft_to_native(pos);
	pos *= vec2(w,h);

	vec4 texel = texture(tex, pos);
	fragColor = vec4(texel.r) * float(pos.x < w && pos.y < h)*t*(2-t);
}
