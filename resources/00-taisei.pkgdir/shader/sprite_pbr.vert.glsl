#version 330

#include "lib/render_context.glslh"
#include "interface/sprite_pbr.glslh"

void main(void) {
	pos = (spriteVMTransform * vec4(vertPos, 0.0, 1.0)).xyz;
	normal = normalize(mat3(spriteVMTransform) * vertNormal);
	tangent = normalize(mat3(spriteVMTransform) * vertTangent.xyz);
	bitangent = normalize(mat3(spriteVMTransform) * cross(vertNormal.xyz, vertTangent.xyz) * vertTangent.w);

	gl_Position = r_projectionMatrix * vec4(pos, 1.0);
	texCoord = uv_to_region(spriteTexRegion, vertTexCoord);
	texCoordRaw = vertTexCoord;
	texCoordOverlay = (spriteTexTransform * vec4(vertTexCoord, 0.0, 1.0)).xy;
	texRegion = spriteTexRegion;
	dimensions = spriteDimensions;
	customParams = spriteCustomParams;
}
