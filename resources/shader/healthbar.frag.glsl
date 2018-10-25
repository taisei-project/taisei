#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/healthbar.glslh"

float sdf(float s, float x) {
    return smoothstep(0.5 - s, 0.5 + s, x);
}

void main(void) {
    vec2 uv = texCoord;
    float hp = fill.x;
    float alt = fill.y;

    vec2 uvShifted = uv - 0.5;
    float centerDist = length(uvShifted);
    float angle = atan(uvShifted.x, -uvShifted.y);

    float normAngle = 0.5 * angle / pi + 0.5;
    float normMinAngle = fillNormAngles.x;
    float normMaxAngle = fillNormAngles.y;
    float normMinAltAngle = fillNormAngles.z;
    float normMaxAltAngle = fillNormAngles.w;

    float globalDist = 1.08;
    float iDist = globalDist / 0.90;
    float oDist = globalDist / 0.96;
    float circleW = 1.01;

    float smoothing = fwidth(uv.x) * globalDist;

    float i0raw = iDist * centerDist;
    float i0 = sdf(smoothing, i0raw);
    float i1raw = (iDist / circleW) * centerDist;
    float i1 = sdf(smoothing, i1raw);
    float inner = i0 - i1;

    float o0raw = oDist * centerDist;
    float o0 = sdf(smoothing, o0raw);
    float o1raw = (oDist / circleW) * centerDist;
    float o1 = sdf(smoothing, o1raw);
    float outer = o0 - o1;

    float glow = sdf(0.05, i0raw) - sdf(0.05, o1raw);

    float fillShrink = 0.01;
    float fillFactor = sdf(0.01, i0raw * (1.0 - fillShrink)) - sdf(0.01, o1raw * (1.0 + fillShrink));

    float fillEdgeFactor;
    float fillTrail = max(0.02 * min(1.0, hp * 32.0), 0.00001);
    fillEdgeFactor  = smoothstep(0.5 + fillTrail, 0.5, normMinAngle - normAngle + 0.5);
    fillEdgeFactor *= smoothstep(0.5 - fillTrail, 0.5, normMaxAngle - normAngle + 0.5);

    float coreFactor = pow(fillFactor, 4.0) * fillEdgeFactor;
    fillFactor *= fillEdgeFactor;

    float altFactor;
    float altTrail = max(0.02 * min(1.0, alt * 32.0), 0.00001);
    altFactor  = smoothstep(0.5 + altTrail, 0.5, normMinAltAngle - normAngle + 0.5);
    altFactor *= smoothstep(0.5 - altTrail, 0.5, normMaxAltAngle - normAngle + 0.5);

    vec4 effectiveFillColor = alphaCompose(mix(fillColor, altFillColor, altFactor), coreFillColor * coreFactor);

    fragColor = alphaCompose(vec4(0), glowColor * glow);
    fragColor = alphaCompose(fragColor, effectiveFillColor * fillFactor);
    fragColor = alphaCompose(fragColor, borderColor * inner);
    fragColor = alphaCompose(fragColor, borderColor * outer);
    fragColor *= opacity;
}
