#version 330 core

#include "lib/render_context.glslh"
#include "interface/sprite_pbr.glslh"

#define features_mask (0)

void main(void) {
	vec4 roughness_sample = texture(tex_roughness, texCoord);
	float alpha = roughness_sample.a;

	if(alpha < PBR_ALPHA_DISCARD_THRESHOLD) {
		discard;
	}

	vec3 ambient = texture(tex_ambient, texCoord).rgb;
	vec3 tbn_normal = sample_normalmap(tex_normal, texCoord);
	mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normalize(normal));

	PBRParams p;
	p.fragPos = pos;
	p.mat.albedo = color.rgb * texture(tex, texCoord).rgb;
	p.mat.roughness = roughness_sample.r;
	p.mat.metallic = customParams.a;
	p.mat.normal = normalize(tbn * tbn_normal);

	PBRState pbr = PBR(p);

	vec3 color = customParams.rgb * ambient;
	color += PBR_PointLights(pbr, light_count, light_positions, light_colors);
	color = PBR_TonemapDefault(color);
	color = PBR_GammaCorrect(color);

	fragColor = vec4(color, 1) * alpha;
}
