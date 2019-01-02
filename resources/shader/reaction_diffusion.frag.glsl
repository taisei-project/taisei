#version 330 core

#include "interface/standard.glslh"
#include "lib/util.glslh"
#include "lib/render_context.glslh"

UNIFORM(1) float dt;
UNIFORM(2) vec2 diffusion_coefficient;
UNIFORM(4) float F;
UNIFORM(5) float K;

void main(void) {
	vec2 uv = texCoord;

	float dx = 2.0/800;//dFdx(uv.x);
	float dy = 2.0/600;//dFdy(uv.y);


	mat3 kernel = mat3(
		0.05, 0.2, 0.05,
		0.2, -1, 0.2,
		0.05, 0.2, 0.05
	);

	vec2 laplacian = vec2(0);
	for(int i = -1; i <= 1; i++) {
		for(int j = -1; j <= 1; j++) {
			laplacian += kernel[i+1][j+1]*texture(tex, uv + vec2(dx*i,dy*j)).rg;
		}
	}
	vec2 prev = texture(tex, uv).rg;
/*
	vec2 t0p = texture(tex, uv + vec2(0, dy)).rg; 
	vec2 t0m = texture(tex, uv + vec2(0, -dy)).rg; 
	vec2 tp0 = texture(tex, uv + vec2(dx, 0)).rg; 
	vec2 tm0 = texture(tex, uv + vec2(-dx, 0)).rg; 


	vec2 laplacian = t0p+t0m+tp0+tm0-4*prev;
*/
	float k = K+0.01*uv.x;
	float f = F+0.01*uv.y;
	
	fragColor.rg = prev + dt * (diffusion_coefficient*laplacian + vec2(-1, 1)*prev.x*prev.y*prev.y + vec2(f * (1 - prev.x), - (k + f) * prev.y));
	fragColor.b = 1*laplacian.g;
	fragColor.a = 1;
}
