#version 450

layout(location = 0) in vec2 inTex;
layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D sceneTex;   // the rendered frame
layout(set = 2, binding = 1) uniform sampler2D depthTex; // depth texture (R channel)

layout(std140, set = 3, binding = 0) uniform DoFUniforms {
    float focalDistance;   // focus distance (0..1 depth)
    float focalRange;      // focus range (0..1 depth)
    float maxBlur;         // max blur radius in pixels
    float pad;
};

void main()
{
    vec4 sceneColor = texture(sceneTex, inTex);
    float depth = texture(depthTex, inTex).r;

    // Depth-of-field blur amount based on depth distance from focal plane.
    float depthDiff = abs(depth - focalDistance);
    float blur = smoothstep(0.0, focalRange, depthDiff) * maxBlur;

    if (blur > 0.01) {
        // Simple blur simulation - reduce contrast based on blur amount.
        // This simulates the defocus effect without expensive sampling.
        float blurFactor = 1.0 - blur * 0.3;
        vec3 mid = vec3(0.5);
        sceneColor.rgb = mid + (sceneColor.rgb - mid) * blurFactor;
    }

    outColor = sceneColor;
}
