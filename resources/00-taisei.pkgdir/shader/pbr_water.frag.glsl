#version 330 core

#define PARALLAX_MIN_LAYERS 6
#define PARALLAX_MAX_LAYERS 8

#define STAGE2_OPTIMIZATIONS

#ifdef STAGE2_OPTIMIZATIONS
// hardcoded assumptions in context of stage 2
// these eliminate some uniform branches, so it doesn't win all that much
#define PBR_CONDITIONAL_FEATURES 0
#define PBR_UNCONDITIONAL_FEATURES (PBR_FEATURE_DIFFUSE_MAP | PBR_FEATURE_NORMAL_MAP | PBR_FEATURE_ENVIRONMENT_MAP | PBR_FEATURE_DEPTH_MAP)
#endif

#include "lib/render_context.glslh"
#include "interface/pbr_water.glslh"
#include "lib/util.glslh"
#include "lib/water.glslh"
#include "lib/frag_util.glslh"

#define IOR_TO_F0(ior) ((ior - 1.0) * (ior - 1.0)) / ((ior + 1.0) * (ior + 1.0))

const float IOR_AIR      = 1.000;
const float IOR_WATER    = 1.333;
const float AIR_TO_WATER = IOR_AIR / IOR_WATER;
const float F0_WATER     = IOR_TO_F0(IOR_WATER);

vec3 topLayer(vec3 normal, vec3 pos, vec3 bottom, float highlight);
vec3 bottomLayer(mat3 tbn, vec2 uv, vec3 pos);

void main(void) {
	vec2 uv = waterTexCoord;

	float height = water_sampleWarpedNoise(uv, mat2(2), vec2(-time, 0));
	vec2 dheightduv = dFduv(height, uv).yx;
	vec2 dheightduv2 = dheightduv * dheightduv;
	float wave_highlight = max(dheightduv2.y - 5 * dheightduv2.x, 0);

	mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normalize(normal));
	vec3 surface_normal = normalize(vec3(-wave_height * dheightduv, 1));

	vec3 bottom;

	if(water_has_bottom_layer == 0) {
		bottom = water_color * 0.05;
	} else {
		vec3 penetration_vec = refract(normalize(pos), tbn * surface_normal, AIR_TO_WATER);
		penetration_vec = transpose(tbn) * penetration_vec;
		penetration_vec /= abs(penetration_vec.z);
		penetration_vec = tbn * penetration_vec;

		vec3 bottom_pos = pos + water_depth * penetration_vec;
		vec2 bottom_uv = wave_offset + (inverse_modelview * vec4(bottom_pos, 1.0)).xy;
		bottom = bottomLayer(tbn, bottom_uv, bottom_pos);
	}

	vec3 combined = topLayer(tbn * surface_normal, pos, bottom, wave_highlight);
	PBR_Generic_MaybeTonemap(combined, features_mask);
	fragColor = vec4(combined, 1.0);
}

vec3 topLayer(vec3 normal, vec3 pos, vec3 bottom, float h) {
	PBRParams p;
	p.fragPos = pos;
	p.inv_camera_transform = inv_camera_transform;
	p.mat.albedo = mix(water_color, water_color + wave_highlight_color, h);
	p.mat.roughness = 0.1;
	p.mat.metallic = 0;
	p.mat.normal = normal;
	p.mat.ambient_occlusion = 1;

	PBRState pbr = PBR(p);
	pbr.F0 = vec3(F0_WATER);

	vec3 color = PBR_PointLights(pbr, light_count, light_positions, light_colors);
	color += PBR_Generic_EnvironmentLight(pbr, ibl_brdf_lut, environment_map, environmentRGB_depthScale.rgb, features_mask);

	return mix(bottom, color, pbr.fresnelNV);
}

vec3 bottomLayer(mat3 tbn, vec2 uv, vec3 pos) {
	PBRParams p;
	p.fragPos = pos;
	p.inv_camera_transform = inv_camera_transform;
	PBR_Interface_LoadMaterial(p.mat, uv, pos, tbn, features_mask);

	PBRState pbr = PBR(p);

	vec3 color = p.mat.ambient;
	color += PBR_PointLights(pbr, light_count, light_positions, light_colors);
	color += PBR_Generic_EnvironmentLight(pbr, ibl_brdf_lut, environment_map, environmentRGB_depthScale.rgb, features_mask);

	return color;
}
