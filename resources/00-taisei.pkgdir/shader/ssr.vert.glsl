#version 330

#include "lib/render_context.glslh"
#include "interface/ssr.glslh"

void main(void) {
	gl_Position = r_projectionMatrix * r_modelViewMatrix * vec4(position,1.0);
	
	normal = normalize(mat3(r_modelViewMatrix)*normalIn);
	tangent = normalize(mat3(r_modelViewMatrix)*tangentIn.xyz);
	bitangent = normalize(mat3(r_modelViewMatrix)*cross(normalIn.xyz, tangentIn.xyz)*tangentIn.w);

	pos = (r_modelViewMatrix * vec4(position, 1.0)).xyz;
	texCoord = (r_textureMatrix * vec4(texCoordRawIn, 0.0, 1.0)).xy;
	texCoordRaw = texCoordRawIn;
}
