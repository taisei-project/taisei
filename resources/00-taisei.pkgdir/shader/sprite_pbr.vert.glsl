#version 330

#include "lib/render_context.glslh"
#include "interface/sprite_pbr.glslh"

void main(void) {
	mat3 spriteVMTransform3 = mat3(spriteVMTransform);
	pos = (spriteVMTransform * vec4(vertPos, 0.0, 1.0)).xyz;
	normal = normalize(spriteVMTransform3 * vertNormal);
	tangent = normalize(spriteVMTransform3 * vertTangent.xyz);
	bitangent = normalize(spriteVMTransform3 * cross(vertNormal.xyz, vertTangent.xyz) * vertTangent.w);

	gl_Position = r_projectionMatrix * vec4(pos, 1.0);
	texCoord = uv_to_region(spriteTexRegion, vertTexCoord);
	color = spriteRGBA;
	customParams = spriteCustomVec;
}
