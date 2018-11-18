#version 330 core

#include "interface/standard.glslh"
#include "lib/util.glslh"
#include "lib/render_context.glslh"

UNIFORM(1) float time;
UNIFORM(2) sampler2D petals;

float idontlikestep(float x) {
	return 1/(exp(x)+1);
}

void main(void) {
	vec2 uvShifted = texCoordRaw - vec2(0.5);
	vec2 uvRaw = rot(12 * time * (time - 2.)) * uvShifted;
	vec2 uv = (r_textureMatrix * vec4(uvRaw, 0.0, 1.0)).xy;

	float scale = 3 * (0.3 + 0.3 * time);

	vec4 flash = vec4(0.9, 0.8, 1, 0) * idontlikestep(100*(time-0.1*abs(uvShifted.y)));
	
	vec4 petalColor = texture(petals, scale * uv); 
	petalColor.r *= 0.7 - 0.7*time;

	float feedbackFactor = 0.5;
	petalColor *= feedbackFactor;

	// cries of the damned gpus from beyond
	fragColor = texture(tex,texCoord - 0.01 * vec2(cos(10 * uv.x), 1 + sin(17 * uv.y)));
	fragColor += 0.2 * texture(petals, rot(time) * fragColor.rb);
	// mix with a drizzle of olive oil
	fragColor = fragColor * (1 - petalColor.a) + petalColor;
	fragColor *= r_color;
	fragColor += flash;
}
