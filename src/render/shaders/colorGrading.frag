#version 450

layout(location = 0) in vec2 inTex;
layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D scene;   // the rendered frame

layout(std140, set = 3, binding = 0) uniform ColorGradingUniforms {
    float temperature;   // -1..1: cool to warm
    float tint;          // -1..1: green to magenta
    float saturation;    // 0..2: 0 = grayscale, 1 = normal, 2 = oversaturated
    float contrast;      // 0..2: 0 = flat, 1 = normal, 2 = high contrast
};

vec3 rgbToHsl(vec3 c)
{
    float maxC = max(c.r, max(c.g, c.b));
    float minC = min(c.r, min(c.g, c.b));
    float chroma = maxC - minC;
    float l = (maxC + minC) / 2.0;
    float h = 0.0, s = 0.0;
    if (chroma > 0.0) {
        if (maxC == c.r) h = mod((c.g - c.b) / chroma, 6.0);
        else if (maxC == c.g) h = (c.b - c.r) / chroma + 2.0;
        else h = (c.r - c.g) / chroma + 4.0;
        s = chroma / (1.0 - abs(2.0 * l - 1.0));
    }
    h /= 6.0;
    return vec3(h, s, l);
}

vec3 hslToRgb(vec3 c)
{
    float h = c.r, s = c.g, l = c.b;
    float m2 = l <= 0.5 ? l * (1.0 + s) : l + s - l * s;
    float m1 = l * 2.0 - m2;
    vec3 v = vec3(m1 + (m2 - m1) * max(0.0, mod(h + 1.0 / 3.0, 1.0)),
                  m1 + (m2 - m1) * max(0.0, mod(h, 1.0)),
                  m1 + (m2 - m1) * max(0.0, mod(h - 1.0 / 3.0, 1.0)));
    return v;
}

void main()
{
    vec4 sceneColor = textureLod(scene, inTex, 0.0);
    vec3 c = sceneColor.rgb;

    // Temperature: shift toward blue (cool) or yellow/orange (warm)
    float temp = temperature * 0.5;
    c.r += temp * 0.1;
    c.b -= temp * 0.1;
    c = clamp(c, 0.0, 1.0);

    // Tint: shift toward green or magenta
    c.g += tint * 0.1;
    c.r -= tint * 0.05;
    c.b -= tint * 0.05;
    c = clamp(c, 0.0, 1.0);

    // Convert to HSL for saturation/contrast adjustments
    vec3 hsl = rgbToHsl(c);
    hsl.g *= saturation; // saturation
    c = hslToRgb(hsl);
    c = clamp(c, 0.0, 1.0);

    // Contrast around middle gray
    float mid = 0.5;
    c = mid + (c - mid) * contrast;
    c = clamp(c, 0.0, 1.0);

    outColor = vec4(c, sceneColor.a);
}
