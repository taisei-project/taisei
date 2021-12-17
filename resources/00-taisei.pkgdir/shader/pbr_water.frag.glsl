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

vec3 topLayer(vec3 normal, vec3 pos, vec3 bottom);
vec3 bottomLayer(mat3 tbn, vec2 uv, vec3 pos);

void main(void) {
	vec2 uv = rot(pi/2) * (texCoord + vec2(-0*time, 0));

	float height = warpedNoise(uv, time);
	vec2 dheightduv = wave_height * dFduv(height, uv.xy);

	mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normalize(normal));
	vec3 surface_normal = normalize(vec3(-dheightduv, 1));

	vec3 penetration_vec = refract(normalize(pos), tbn * surface_normal, AIR_TO_WATER);
	penetration_vec = transpose(tbn) * penetration_vec;
	penetration_vec /= abs(penetration_vec.z);
	penetration_vec = tbn * penetration_vec;

	vec3 bottom_pos = pos + water_depth * penetration_vec;
	vec2 bottom_uv = wave_offset +(inverse_modelview * vec4(bottom_pos, 1.0)).xy;

	vec3 bottom = bottomLayer(tbn, bottom_uv, bottom_pos);
	vec3 combined = topLayer(tbn * surface_normal, pos, bottom);

	PBR_Generic_MaybeTonemap(combined, features_mask);

	fragColor = vec4(combined, 1.0);
}

vec3 topLayer(vec3 normal, vec3 pos, vec3 bottom) {

	PBRParams p;
	p.fragPos = pos;
	p.inv_camera_transform = inv_camera_transform;
	p.mat.roughness = 0.1;
	p.mat.metallic = 0;
	p.mat.normal = normal;
	p.mat.ambient_occlusion = 1;

	PBRState pbr = PBR(p);
	pbr.F0 = vec3(F0_WATER);

	vec3 color = PBR_PointLights(pbr, light_count, light_positions, light_colors);
	color += PBR_Generic_EnvironmentLight(pbr, ibl_brdf_lut, environment_map, features_mask);

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
	color += PBR_Generic_EnvironmentLight(pbr, ibl_brdf_lut, environment_map, features_mask);

	return color;
}
