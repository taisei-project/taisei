#version 330 core

#include "lib/render_context.glslh"
#include "interface/ssr.glslh"
#include "lib/util.glslh"

UNIFORM(4) mat4 inv_camera_transform;

void main(void) {
	vec3 reflected_ray = mat3(inv_camera_transform) * reflect(pos, normal);
	fragColor = texture(tex, equirectMapCoord(reflected_ray));

	fragColor.r = linear_to_srgb(fragColor.r);
	fragColor.g = linear_to_srgb(fragColor.g);
	fragColor.b = linear_to_srgb(fragColor.b);
}
