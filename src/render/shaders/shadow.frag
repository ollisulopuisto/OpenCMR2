#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 shadowUV;
layout(location = 1) in float shadowAlpha;
layout(set=2,binding=0) uniform sampler2D coverage;
layout(std140,set=3,binding=0) uniform ShadowCoverage { vec4 settings; };


void main()
{
    if(settings.x>0.5){
        float alpha=texture(coverage,shadowUV).a*(settings.w>0.5?shadowAlpha:1.0);
        if(settings.z>0.5?alpha<settings.y:alpha<=settings.y)discard;
    }
    // Write depth (gl_FragCoord.z in [0,1]) to the R channel of shadowMap,
    // so it can be sampled by the main pass for shadow comparison.
    outColor = vec4(gl_FragCoord.z, 0.0, 0.0, 1.0);
}
