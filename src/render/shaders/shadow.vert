#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"

layout(location = 0) in vec3 inPosition;

void main()
{
    // Model -> world -> light clip space. Only depth matters here.
    vec4 worldPos = world * vec4(inPosition, 1.0);
    gl_Position = lightViewProj * worldPos;
}
