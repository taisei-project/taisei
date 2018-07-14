#version 330 core

#include "lib/util.glslh"
#include "interface/standard.glslh"

UNIFORM(1) float t;

void main(void) {
	vec2 r = vec2(texCoord.x - 0.5, 1.0 - texCoord.y);

	// this is > 0 in the basic core region of the spark. to make the shape more interesting, it is modulated with additional terms later.
	float coreRegion = 0.4 * pow(r.y, 0.3) - abs(r.x);

	// random rainbow pattern moving with time
	vec3 clr = vec3(0.7+0.3*sin(r.x*2.),0.7+0.3*cos(r.y*2.-t*0.1),0.7+0.3*sin((r.x-r.y)*1.4+t*0.2));


	// _smoothstep(coreRegion, 0) would be a mask for the bare cone shape of the spark. By adding extra terms it is distorted.
	// sin adds a wavy edge, the fraction with t takes care of fading in/out

	float colorRegion = coreRegion - 0.01 * sin(r.y * 30 - t * 0.7) * (1-1/(t+1)) - 0.05;

	float whiteRegion = coreRegion - 0.15 - 0.2 / (1 + t * 0.1);

	fragColor = vec4(clr, 0)     * _smoothstep(colorRegion, 0.05)
	          + vec4(1, 1, 1, 0) * _smoothstep(whiteRegion, 0.02);
}
