#version 330 core

#include "lib/render_context.glslh"
#include "interface/sprite_pbr.glslh"

void main(void) {
	vec4 roughness_sample = texture(tex_roughness, texCoord);
	float alpha = roughness_sample.a;

	if(alpha < 0.3) {
		discard;
	}

	vec3 ambient = texture(tex_ambient, texCoord).rgb;
	vec3 tbn_normal = sample_normalmap(tex_normal, texCoord);
	mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normalize(normal));

	PBRParams p;
	p.fragPos = pos;
	p.albedo = color.rgb * texture(tex, texCoord).rgb;
	p.roughness = roughness_sample.r;
	p.metallic = customParams.a;
	p.normal = normalize(tbn * tbn_normal);

	PBRState pbr = PBR(p);

	vec3 Lo = vec3(0.0);
	for(int i = 0; i < light_count; ++i) {
		Lo += PBR_PointLight(pbr, PointLight(light_positions[i], light_colors[i]));
	}

	vec3 ambient_color = customParams.rgb;
	vec3 color = ambient * ambient_color + Lo;
	color = PBR_TonemapReinhard(color);
	color = PBR_GammaCorrect(color);

	fragColor = vec4(color, 1) * alpha;
}
