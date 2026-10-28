#version 450

layout(location = 0) in vec2 inTex;
layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D source;

layout(std140, set = 3, binding = 0) uniform BlurUniforms {
    vec2 direction;   // (1,0) horizontal, (0,1) vertical
    float offsetScale;// multiplier on the one-texel step
    int samples;      // taps each side of centre
    float pad;
};

// Fixed 5-tap Gaussian-ish weights: centre 0.4, then 0.25, 0.125 each side.
const float w[5] = float[5](0.4, 0.25, 0.125, 0.0625, 0.03125);

void main()
{
    vec2 size = vec2(textureSize(source, 0));
    vec2 step = direction / size * offsetScale;
    vec4 color = texture(source, inTex) * w[0];
    for (int i = 1; i < samples && i < 5; i++) {
        color += texture(source, inTex - step * float(i)) * w[i];
        color += texture(source, inTex + step * float(i)) * w[i];
    }
    outColor = color;
}
