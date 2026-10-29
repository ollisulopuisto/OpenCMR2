#pragma once
// Native GL coordinates: keep stored rows unchanged. Unlike the translated
// Vulkan vertex shader this needs no Y flip. UV covers the WHOLE source.
static const unsigned char glEffectsVertex[] = R"GLSL(#version 330
out vec2 varying0;
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0, 1);
    varying0 = p;
}
)GLSL";
static const unsigned char glWeatherVertex[] = R"GLSL(#version 330
layout(location=0) in vec2 pos;
layout(location=1) in vec4 color;
out vec4 particleColor;
void main() { gl_Position=vec4(pos*2.0-1.0,0,1); particleColor=color; }
)GLSL";
static const unsigned char glWeatherFragment[] = R"GLSL(#version 330
in vec4 particleColor;
out vec4 outColor;
void main() { outColor=particleColor; }
)GLSL";
