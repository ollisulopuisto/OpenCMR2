#version 450

layout(location = 0) in vec2 inTex;
layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D scene;   // the rendered frame

layout(std140, set = 3, binding = 0) uniform BrightPassUniforms {
    float threshold;   // 0..1: pixels brighter than this (luma) pass through
    float softness;    // width of the smoothstep knee above threshold
    float pad[2];
};

void main()
{
    vec4 c = texture(scene, inTex);
    float lum = dot(c.rgb, vec3(0.2126, 0.7152, 0.0722));
    float bright = smoothstep(threshold, threshold + softness, lum);
    outColor = vec4(c.rgb * bright, c.a * bright);
}
