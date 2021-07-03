#version 330 core

#include "lib/render_context.glslh"
#include "interface/pbr.glslh"

UNIFORM(20) sampler2D envmap;
UNIFORM(21) mat4 inv_camera_transform;

void main(void) {
	vec4 roughness_sample = texture(roughness_map, texCoord);
	float alpha = roughness_sample.a;

	if(alpha < 0.3) {
		discard;
	}

	vec3 ambient = texture(ambient_map, texCoord).rgb;
	vec3 tbn_normal = sample_normalmap(normal_map, texCoord);
	mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normalize(normal));

	PBRParams p;
	p.fragPos = pos;
	p.albedo = r_color.rgb * texture(tex, texCoord).rgb;
	p.roughness = roughness_sample.r;
	p.metallic = metallic;
	p.normal = normalize(tbn * tbn_normal);

	PBRState pbr = PBR(p);

	vec3 Lo = vec3(0.0);
	for(int i = 0; i < light_count; ++i) {
		Lo += PBR_PointLight(pbr, PointLight(light_positions[i], light_colors[i]));
	}

	vec3 color = ambient * ambient_color + Lo;

	vec3 reflected_ray = mat3(inv_camera_transform) * reflect(pos, p.normal);
	vec2 texCoord = equirectMapCoord(reflected_ray);

	float r = 1 - p.roughness;
	color += (r * r) * texture(envmap, texCoord).rgb * mix(vec3(0.5), p.albedo, p.metallic);

	color = PBR_TonemapReinhard(color);
	color = PBR_GammaCorrect(color);

	fragColor = vec4(color, 1) * alpha;
}
