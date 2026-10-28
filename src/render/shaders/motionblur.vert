#version 450
// A triangle covering the viewport: gl_VertexIndex picks a corner of the
// screen-space triangle strip. Texture coordinates pass straight through
// so the fragment shader can sample the current and previous frames at the
// same pixel.
layout(location = 0) out vec2 outTex;

void main()
{
    vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
    outTex = p * 0.5;
}
