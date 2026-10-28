#version 450

layout(location = 0) in vec2 inTex;
layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D scene;   // the rendered frame
layout(set = 2, binding = 1) uniform sampler2D depthTex; // depth buffer (R channel)

layout(std140, set = 3, binding = 0) uniform GodRaysUniforms {
    float intensity;   // 0..1: god rays strength
    float pad[3];
};

void main()
{
    vec4 sceneColor = textureLod(scene, inTex, 0.0);
    // Ray-march from this pixel toward the light source (sun is above-right).
    // Accumulate light along the ray where it's not occluded by depth.
    vec2 lightDir = normalize(vec2(0.35, -0.8)); // sun direction in screen space
    float maxDist = 100.0;
    int steps = 32;
    float step = maxDist / float(steps);
    float accum = 0.0;
    vec2 pos = inTex;
    float startDepth = textureLod(depthTex, inTex, 0.0).r;

    for (int i = 0; i < steps; i++) {
        pos += lightDir * step * 0.002; // step in screen space
        if (pos.x < 0.0 || pos.x > 1.0 || pos.y < 0.0 || pos.y > 1.0)
            break;
        float sampleDepth = textureLod(depthTex, pos, 0.0).r;
        // If the sample point is in front of the current depth, it's occluded.
        if (sampleDepth < startDepth)
            break;
        accum += 1.0;
    }

    float godRay = accum / float(steps) * intensity;
    sceneColor.rgb += godRay * vec3(1.0, 0.95, 0.8); // warm sun tint

    outColor = sceneColor;
}
