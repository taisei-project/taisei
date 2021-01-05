#version 330 core

#include "lib/util.glslh"
#include "lib/render_context.glslh"
#include "interface/pbr.glslh"

UNIFORM(1) sampler2D roughness_map;
UNIFORM(2) sampler2D normal_map;
UNIFORM(3) sampler2D ambient_map;
UNIFORM(4) float metallic;
UNIFORM(5) int light_count;
UNIFORM(6) vec3 ambient_color; // modulates ambient map
#define MAX_LIGHT_COUNT 6

UNIFORM(7) vec3 light_positions[MAX_LIGHT_COUNT];
UNIFORM(13) vec3 light_colors[MAX_LIGHT_COUNT]; // layout-id also depends on MAX_LIGHT_COUNT


// taken from https://learnopengl.com/PBR/Lighting
float distribution_ggx(float ndoth, float roughness) {
	float a = roughness*roughness;

	float a2 = a*a;
	float ndoth2 = ndoth*ndoth;

	float num = a2;
	float denom = ndoth2 * (a2 - 1.0) + 1.0;
	denom = pi * denom * denom;

	return num / denom;
}

float geometry_schlick_ggx(float ndotv, float roughness) {
	float r = (roughness + 1.0);
	float k = r * r * 0.125;

	float num   = ndotv;
	float denom = mix(ndotv, 1, k);

	return num / denom;
}

float geometry_smith(float ndotv, float ndotl, float roughness) {
	float ggx2 = geometry_schlick_ggx(ndotv, roughness);
	float ggx1 = geometry_schlick_ggx(ndotl, roughness);

	return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cosTheta, vec3 F0) {
	float x = 1.0 - cosTheta;
	float x2 = (x * x);
	float x5 = x2 * x2 * x;

    return mix(vec3(x5), vec3(1.0), F0);
}

void main(void) {
	vec4 roughness_sample = texture(roughness_map, texCoord);
	float alpha = roughness_sample.a;

	if(alpha < 0.3) {
		discard;
	}

	vec3 albedo = (r_color * texture(tex, texCoord)).rgb;
	float roughness = roughness_sample.r;
	vec3 tbn_normal = sample_normalmap(normal_map, texCoord);
	vec3 ambient = texture(ambient_map, texCoord).rgb;

	vec3 n = normalize(mat3(normalize(tangent), normalize(bitangent), normalize(normal))*tbn_normal);
	vec3 v = normalize(-pos);
	float NdotV = max(dot(n, v), 0.0);

	vec3 F0 = mix(vec3(0.04), albedo, metallic);

	float one_minus_metallic = 1.0 - metallic;
	const float inverse_pi = 1.0 / pi;

	vec3 Lo = vec3(0.0);
	for(int i = 0; i < light_count; ++i) {
		vec3 cam_to_light = light_positions[i] - pos;

		vec3 l = normalize(cam_to_light);
		vec3 h = normalize(v + l);
		vec3 radiance = light_colors[i] / dot(cam_to_light, cam_to_light);

		float NdotL = max(dot(n, l), 0.0);
		float NdotH = max(dot(n, h), 0.0);
		float HdotV = max(dot(h, v), 0.0);

		float NDF = distribution_ggx(NdotH, roughness);
		float G = geometry_smith(NdotV, NdotL, roughness);
		vec3 F = fresnel_schlick(HdotV, F0);

		vec3 kS = F;
		vec3 kD = (1.0 - kS) * one_minus_metallic;

		vec3 numerator = NDF * G * F;
		float denominator = 4.0 * NdotV * NdotL;
		vec3 specular = numerator / max(denominator, 0.001);

		Lo += (kD * albedo * inverse_pi + specular) * radiance * NdotL;
	}

	//vec3 ambient = vec3(0.03) * albedo * ao;
	vec3 color = ambient_color*ambient + Lo;

	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2));

	fragColor = vec4(color, 1)*alpha;
}
