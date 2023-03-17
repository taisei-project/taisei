#version 330

#include "lib/render_context.glslh"
#include "interface/pbr_water.glslh"
#include "lib/util.glslh"

void main(void) {
	pos = (r_modelViewMatrix * vec4(position,1.0)).xyz;
	gl_Position = r_projectionMatrix * vec4(pos, 1.0);

	mat3 mv3 = mat3(r_modelViewMatrix);
	normal = normalize(mv3 * normalIn);
	tangent = normalize(mv3 * tangentIn.xyz);
	bitangent = normalize(mv3 * cross(normalIn.xyz, tangentIn.xyz) * tangentIn.w);

	texCoord = wave_scale * flip_native_to_bottomleft(texCoordRawIn - wave_offset);
	waterTexCoord = texCoord + vec2(-time, 0);
}
