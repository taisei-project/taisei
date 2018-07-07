#version 330 core

#include "lib/defs.glslh"
#include "lib/render_context.glslh"
#include "interface/standard.glslh"

UNIFORM(1) float tbomb;

void main(void) {
	vec2 pos = texCoordRaw;

	// During the end of the bomb, the screen splits and the originalColor is revealed again
	float inSplitArea = step(-pos.y * pos.y - abs(pos.x - 0.5) + tbomb * (tbomb-0.8) * 10, 0);
	vec3 originalColor = texture(tex, texCoord).rgb;

	// In the remaining area, the left and right half get shifted up/down
	float shift = (2 * float(pos.x<0.5) - 1) * 0.1 * sqrt(tbomb);
	pos.y = pos.y * (1 - abs(shift)) + float(shift > 0) * shift;
	vec3 shiftedColor = texture(tex, vec2(r_textureMatrix * vec4(pos, 0, 1))).rgb;

	shiftedColor -= vec3(+shift,0.01*tbomb,-shift); // red/blue shift
	shiftedColor *= 1 + 4 * tbomb * (1 - pow(abs(pos.x - 0.5), 3)); // boost intensity

	// put everything together applying a tanh against overflows.
	fragColor = vec4(mix(originalColor, tanh(shiftedColor), inSplitArea), 1);
}
