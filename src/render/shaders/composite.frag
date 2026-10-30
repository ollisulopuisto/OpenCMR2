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
    vec3 c = s.rgb + b.rgb * intensity;
    // Soft shoulder above 0.8 so bloom added to an already bright sky compresses instead of clipping to white.
    // Continuous with slope 1 at 0.8; values at or below 0.8 are unchanged.
    const float knee = 0.8;
    vec3 t = max(c - knee, vec3(0.0)) / (1.0 - knee);
    c = mix(c, vec3(knee) + (1.0 - knee) * t / (1.0 + t), step(vec3(knee), c));
    outColor = vec4(c, s.a);
}
