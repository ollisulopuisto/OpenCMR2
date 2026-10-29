#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 4) in vec2 inTex;
layout(location = 2) in vec4 inDiffuse;
layout(location = 1) out float shadowAlpha;
layout(location = 0) out vec2 shadowUV;

void main()
{
    // Model -> world -> light clip space. Only depth matters here.
    vec4 worldPos = world * vec4(inPosition, 1.0);
    gl_Position = lightViewProj * worldPos;
    shadowUV = inTex;
    shadowAlpha = inDiffuse.a;
}
