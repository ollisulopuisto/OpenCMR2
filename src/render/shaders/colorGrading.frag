#version 450
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D scene;
layout(std140, set = 3, binding = 0) uniform ColorGradingUniforms {
    float temperature;
    float tint;
    float saturation;
    float contrast;
};
void main()
{
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(scene, 0));
    vec4 center = texture(scene, uv);
    // Analytic grade, not a LUT. Neutral settings are exactly the identity.
    // Multiplicative balance avoids lifting black into a colored fog.
    vec3 balance = vec3(1.0 + temperature * 0.08 + tint * 0.04,
                        1.0 - tint * 0.08,
                        1.0 - temperature * 0.08 + tint * 0.04);
    vec3 c = center.rgb * balance;
    float y = dot(c, vec3(0.2126, 0.7152, 0.0722));
    c = mix(vec3(y), c, saturation);
    c = (c - 0.5) * contrast + 0.5;
    outColor = vec4(clamp(c, 0.0, 1.0), center.a);
}
