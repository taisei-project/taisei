#version 330 core

#include "lib/render_context.glslh"
#include "lib/util.glslh"
#include "interface/healthbar.glslh"

float sdf(float s, float x) {
    return smoothstep(0.5 - s, 0.5 + s, x);
}

float capsule(vec2 p, vec2 a, vec2 b, float r) {
    vec2 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return 1.0 - length(pa - ba * h) / r;
}

void main(void) {
    // You think you can just go ahead and tweak those magic numbers as you please?
    // Well good damn luck with that.

    vec2 uv = texCoord;
    float hp = fill.x;
    float alt = fill.y;

    vec2 primSize = 1.0 / fwidth(uv);
    vec2 fragCoord = uv * primSize;

    float barHeightPx = primSize.y * 0.238;
    float lineWidthPx = primSize.y / 24.0;
    float barOffsetPx = barHeightPx + primSize.x / 96.0;

    float offsetFactor = (barOffsetPx / primSize.x);
    hp = (offsetFactor + hp * (1.0 - offsetFactor)) * step(0.001, hp);
    alt = (offsetFactor + alt * (1.0 - offsetFactor)) * step(0.001, alt);

    vec2 barLeft = vec2(barOffsetPx, primSize.y * 0.5);
    vec2 barRight = vec2(primSize.x - barOffsetPx, primSize.y * 0.5);

    float borderInner = capsule(fragCoord, barLeft, barRight, barHeightPx - lineWidthPx);
    float borderOuter = capsule(fragCoord, barLeft, barRight, barHeightPx + lineWidthPx);
    float borderOutline = min(borderOuter, 1.0 - borderInner);
    float border = sdf(fwidth(borderOutline), borderOutline);

    float glow = sdf(0.75, borderOuter * 0.5) * 1.25;

    float fillShrink = 0.01;
    float fillSmoothing = 0.3;
    float fillFactor = sdf(fillSmoothing, borderInner * (1.0 - fillShrink));

    float edgeSmoothing = max(0.02 * min(1.0, fill.x * 32.0), 0.00001);
    float fillEdgeFactor = sdf(edgeSmoothing, hp - uv.x + 0.5);
    float altFactor = sdf(edgeSmoothing, alt - uv.x + 0.5);

    vec4 effectiveFillColor = mix(fillColor, altFillColor, altFactor);
    effectiveFillColor = alphaCompose(effectiveFillColor, coreFillColor * (pow(fillFactor, 4.0) * fillEdgeFactor));

    fillFactor *= fillEdgeFactor;

    fragColor = vec4(0);
    fragColor = alphaCompose(fragColor, glowColor * glow);
    fragColor = alphaCompose(fragColor, effectiveFillColor * fillFactor);
    fragColor = alphaCompose(fragColor, borderColor * border);
    fragColor *= opacity;
}
