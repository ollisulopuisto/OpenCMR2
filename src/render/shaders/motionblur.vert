#version 450
// A triangle covering the viewport: gl_VertexIndex picks a corner of the
// screen-space triangle. SDL_GPU clip space has +Y up, but texture rows
// start at the top (+V down), including Metal render targets. Flip only the
// varying V so every post-process input samples the same screen pixel.
// Native OpenGL uses its separate vertex shader in gl_effects_sources.h.
layout(location = 0) out vec2 outTex;

void main()
{
    vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
    outTex = vec2(p.x, 1.0 - p.y);
}
