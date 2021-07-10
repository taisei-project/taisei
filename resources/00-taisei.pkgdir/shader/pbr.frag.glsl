#version 330 core

#include "lib/render_context.glslh"
#include "interface/pbr.glslh"

void main(void) {
	float roughness = ambientRGB_roughnessA.a;
	float alpha;

	if(bool(features_mask & PBR_FEATURE_ROUGHNESS_MAP)) {
		vec4 roughness_sample = texture(roughness_map, texCoord);
		roughness *= roughness_sample.r;
		alpha = roughness_sample.a;

		// TODO: a way to opt out of this, since it hurts performance.
		if(alpha < 0.3) {
			discard;
		}
	} else {
		alpha = 1.0;
	}

	float metallic = diffuseRGB_metallicA.a;
	vec3 diffuse = diffuseRGB_metallicA.rgb;

	if(bool(features_mask & PBR_FEATURE_DIFFUSE_MAP)) {
		diffuse *= texture(diffuse_map, texCoord).rgb;
	}

	vec3 ambient = ambientRGB_roughnessA.rgb;

	if(bool(features_mask & PBR_FEATURE_AMBIENT_MAP)) {
		ambient *= texture(ambient_map, texCoord).rgb;
	}

	vec3 tbn_normal;

	if(bool(features_mask & PBR_FEATURE_NORMAL_MAP)) {
		tbn_normal = sample_normalmap(normal_map, texCoord);
	} else {
		tbn_normal = vec3(0, 0, 1);
	}

	mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normalize(normal));

	PBRParams p;
	p.fragPos = pos;
	p.albedo = diffuse;
	p.roughness = roughness;
	p.metallic = metallic;
	p.normal = tbn * tbn_normal;

	PBRState pbr = PBR(p);

	vec3 Lo = vec3(0.0);
	for(int i = 0; i < light_count; ++i) {
		Lo += PBR_PointLight(pbr, PointLight(light_positions[i], light_colors[i]));
	}

	vec3 color = ambient + Lo;

	if(bool(features_mask & PBR_FEATURE_ENVIRONMENT_MAP)) {
		vec3 reflected_ray = mat3(inv_camera_transform) * reflect(pos, p.normal);
		vec3 reflection = texture(environment_map, fixCubeCoord(reflected_ray)).rgb;
		float r = (1 - p.roughness);
		color += (r * r) * reflection * mix(vec3(0.5), p.albedo, p.metallic);
	}

	color = PBR_TonemapReinhard(color);
	color = PBR_GammaCorrect(color);

	fragColor = vec4(color * alpha, alpha);
}
