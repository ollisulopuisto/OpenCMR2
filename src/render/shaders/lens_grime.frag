#version 450

layout(location = 0) in vec2 inTex;
layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D scene;   // the rendered frame

layout(std140, set = 3, binding = 0) uniform LensGrimeUniforms {
    float aberration;   // 0..1: color-fringing strength toward edges
    float vignette;     // 0..1: corner/edge darkening strength
    float grain;        // 0..1: film grain strength
    float pad;
};

// Deterministic pseudo-random per pixel.
float Rand(vec2 uv)
{
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec4 c = texture(scene, inTex);

    // Chromatic aberration: shift R/G/B texture lookups in opposite
    // directions proportional to distance from screen center.
    vec2 center = vec2(0.5);
    vec2 d = inTex - center;
    float dist = length(d);
    float ab = aberration * dist * 0.02;
    if (ab > 0.0) {
        vec2 dir = d / (dist + 1e-5);
        float r = texture(scene, inTex + dir * ab).r;
        float g = texture(scene, inTex - dir * ab).g;
        float b = texture(scene, inTex).b;
        c = vec4(r, g, b, c.a);
    }

    // Vignetting: darken toward the corners.
    if (vignette > 0.0) {
        float vign = pow(dist * 1.414, 2.0) * vignette;
        c.rgb *= 1.0 - vign;
    }

    // Film grain: subtle per-pixel noise overlay.
    if (grain > 0.0) {
        float n = Rand(inTex * 100.0 + gl_FragCoord.xy * 0.01);
        c.rgb += (n - 0.5) * grain * 0.1;
    }

    outColor = c;
}
