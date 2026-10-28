#version 450

layout(location = 0) in vec2 inTex;
layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D scene;   // the rendered frame
layout(set = 2, binding = 1) uniform sampler2D bloom;   // blurred bright pixels

layout(std140, set = 3, binding = 0) uniform CompositeUniforms {
    float intensity;   // bloom strength multiplier
    float pad[3];
};

void main()
{
    vec4 s = texture(scene, inTex);
    vec4 b = texture(bloom, inTex);
    outColor = vec4(s.rgb + b.rgb * intensity, s.a);
}
