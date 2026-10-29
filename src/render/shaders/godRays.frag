#version 450
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D scene;
layout(set = 2, binding = 1) uniform sampler2D depthTex;
layout(set=2,binding=2) uniform sampler2D cloudTransmission;
layout(std140, set = 3, binding = 0) uniform GodRaysUniforms {
    vec4 settings; // intensity, artistic screen-space sun x/y, unused
};
void main()
{
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(scene, 0));
    vec4 center = texture(scene, uv);
    vec2 delta = (settings.yz - uv) * (0.85 / 32.0);
    vec2 pos = uv;
    vec3 light = vec3(0.0);
    float decay = 1.0;
    float total = 0.0;
    // Bright sky is the light mask. Geometry blocks it instead of receiving
    // an unconditional white veil. Normalization keeps exposure step-independent.
    for (int i = 0; i < 32; ++i) {
        pos += delta;
        if (any(lessThan(pos, vec2(0))) || any(greaterThan(pos, vec2(1)))) break;
        float sky = smoothstep(0.995, 0.9999, texture(depthTex, pos).r);
        vec3 source = texture(scene, pos).rgb;
        float bright = smoothstep(0.55, 0.9, dot(source, vec3(0.299, 0.587, 0.114)));
        light += source * sky * bright * decay * texture(cloudTransmission,pos).a * texture(cloudTransmission,settings.yz).a;
        total += decay;
        decay *= 0.96;
    }
    vec3 shafts = light / max(total, 1.0) * settings.x * 0.2 * vec3(1.0, 0.97, 0.9);
    // Screen blend protects highlight headroom instead of clipping additive white.
    outColor = vec4(vec3(1.0) - (vec3(1.0) - center.rgb) * (vec3(1.0) - shafts), center.a);
}
