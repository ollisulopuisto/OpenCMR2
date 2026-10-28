#version 450

layout(location = 0) in vec2 inTex;
layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D current;   // back buffer: this frame
layout(set = 2, binding = 1) uniform sampler2D previous;  // previous frame

layout(std140, set = 3, binding = 0) uniform MotionBlurUniforms {
    float blurMix;        // 0..1: how much of the previous frame to blend in
    float pad[3];
};

void main()
{
    vec4 c = texture(current, inTex);
    vec4 p = texture(previous, inTex);
    outColor = mix(c, p, blurMix);
}
