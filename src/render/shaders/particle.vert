#version 450
// Screen-space particle billboard. inPosition is normalised screen space
// (0..1, y=0 at the top), inSize is the half-width/height in screen fraction.
// Builds a 6-vertex quad around the particle centre.
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inSize;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outTex;

void main()
{
    const vec2 offsets[6] = vec2[](
        vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0),
        vec2(-1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, 1.0)
    );
    vec2 o = offsets[gl_VertexIndex];
    vec2 p = inPosition + o * inSize;
    // Normalised screen space (y=0 top) to Vulkan clip space (y=1 top).
    gl_Position = vec4(p.x * 2.0 - 1.0, 1.0 - p.y * 2.0, 0.0, 1.0);
    outColor = inColor;
    outTex = o * 0.5 + 0.5;
}
