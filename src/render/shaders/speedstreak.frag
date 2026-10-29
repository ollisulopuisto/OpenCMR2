#version 450
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D scene;
layout(std140, set = 3, binding = 0) uniform SpeedStreakUniforms {
    vec4 settings; // strength 0..1, aspect (width/height), unused, unused
};
// Mirrors src/render/speed_streak.h. Directional radial streak: samples run from the pixel toward the screen centre, so the
// scenery smears outward. The clear inner zone is a radial exclusion, not a car mask.
const float INNER = 0.30;
const float RAMP = 0.30;
const float MAXLEN = 0.10;
const int TAPS = 12;
void main()
{
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(scene, 0));
    vec4 base = texture(scene, uv);
    vec2 c = uv - vec2(0.5);
    float r = length(c * vec2(settings.y, 1.0));
    float w = smoothstep(INNER, INNER + RAMP, r) * clamp(settings.x, 0.0, 1.0);
    if (w <= 0.0) { outColor = base; return; }
    vec2 toCenter = -c * (w * MAXLEN);
    vec3 sum = base.rgb * 2.0;
    float total = 2.0;
    for (int i = 1; i <= TAPS; ++i) {
        float t = float(i) / float(TAPS);
        float k = 1.0 - 0.6 * t;
        sum += texture(scene, uv + toCenter * t).rgb * k;
        total += k;
    }
    outColor = vec4(sum / total, base.a);
}
