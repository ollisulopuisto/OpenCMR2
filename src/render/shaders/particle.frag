#version 450

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inTex;
layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D dot;   // soft white particle dot

void main()
{
    vec4 t = texture(dot, inTex);
    outColor = inColor * t;
}
