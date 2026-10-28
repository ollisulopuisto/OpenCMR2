#version 450

layout(location = 0) out vec4 outColor;

void main()
{
    // Write depth (gl_FragCoord.z in [0,1]) to the R channel of shadowMap,
    // so it can be sampled by the main pass for shadow comparison.
    outColor = vec4(gl_FragCoord.z, 0.0, 0.0, 1.0);
}
