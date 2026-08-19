#version 330 core

#include "lib/sprite_main.frag.glslh"

#include "lib/util.glslh"

void spriteMain(out vec4 fragColor) {
	vec4 mask = texture(tex, texCoord);

	if(mask.a == 0.0) {
		discard;
	}

	float opacity = customVec0.x;
	float shadow_thres = customVec0.y;
	float shadow_strength = customVec0.z;
	float shadow_brightness = customVec0.w;
	vec4 edge_color = color;
	vec4 core_color = customVec1;
	vec4 shifted_color0 = customVec2;
	vec4 shifted_color1 = customVec3;

	float shadow_mask = mask.r;
	float edge_mask = mask.g;
	float core_mask = mask.b;

	edge_color = mix(edge_color, shifted_color0, mask.b);
	edge_color = mix(edge_color, shifted_color1, mask.r);

	float shadow_value = smoothstep(0, shadow_thres, shadow_mask) * shadow_strength;
	vec4 shadow = vec4(edge_color.rgb * shadow_brightness, edge_color.a) * shadow_value;

	fragColor = alphaCompose(shadow, edge_color * edge_mask + core_color * core_mask) * opacity;
}
