#version 450
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D scene;
layout(std140, set = 3, binding = 0) uniform SpeedStreakUniforms {
    vec4 settings; // strength 0..1, aspect (width/height), time in seconds, speed-lines amount 0..1
};
// Mirrors src/render/speed_streak.h. Directional radial streak: samples run from the pixel toward the screen centre, so the
// scenery smears outward. The clear inner zone is a radial exclusion, not a car mask.
const float INNER = 0.30;
const float RAMP = 0.30;
const float MAXLEN = 0.10;
const int TAPS = 12;
// Speed lines (mirrors src/render/speed_lines.h): thin rays that converge on the screen centre, drawn over the frame. Visual only.
const float LINES_RAYS = 96.0;
const float LINES_INNER = 0.34;      // clear zone radius (screen heights, aspect corrected), a little outside the streak's inner zone
const float LINES_FADE = 0.22;       // width of the fade-in from clear to full
float hash1(float n) { return fract(sin(n * 127.1 + 311.7) * 43758.5453); }
float speedLines(vec2 pc, float time)
{
    float r = length(pc);
    float a = atan(pc.y, pc.x) / 6.2831853 + 0.5;       // 0..1 around the centre
    float f = a * LINES_RAYS;
    float id = floor(f);
    float h = hash1(id);
    if (h < 0.45) return 0.0;                              // about half the rays are absent
    float local = abs(fract(f) - 0.5);                     // 0 on the ray's axis, 0.5 between rays
    float halfWidth = mix(0.035, 0.11, clamp(r * 1.2, 0.0, 1.0)); // thin near the centre, wider outward
    float line = 1.0 - smoothstep(halfWidth * 0.35, halfWidth, local);
    float dash = fract(r * mix(2.2, 4.0, hash1(id + 9.0)) - time * (1.6 + 2.4 * h) + hash1(id + 3.0));  // segments run outward
    float seg = smoothstep(0.0, 0.18, dash) * (1.0 - smoothstep(0.55, 0.8, dash));
    float zone = smoothstep(LINES_INNER, LINES_INNER + LINES_FADE, r);
    return line * seg * zone * (0.45 + 0.55 * h);
}
void main()
{
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(scene, 0));
    vec4 base = texture(scene, uv);
    vec2 c = uv - vec2(0.5);
    float r = length(c * vec2(settings.y, 1.0));
    float w = smoothstep(INNER, INNER + RAMP, r) * clamp(settings.x, 0.0, 1.0);
    float lines = clamp(settings.w, 0.0, 1.0) > 0.0 ? speedLines(c * vec2(settings.y, 1.0), settings.z) * clamp(settings.w, 0.0, 1.0) : 0.0;
    if (w <= 0.0) { outColor = vec4(mix(base.rgb, vec3(0.93, 0.95, 1.0), lines * 0.75), base.a); return; }
    vec2 toCenter = -c * (w * MAXLEN);
    vec3 sum = base.rgb * 2.0;
    float total = 2.0;
    for (int i = 1; i <= TAPS; ++i) {
        float t = float(i) / float(TAPS);
        float k = 1.0 - 0.6 * t;
        sum += texture(scene, uv + toCenter * t).rgb * k;
        total += k;
    }
    outColor = vec4(mix(sum / total, vec3(0.93, 0.95, 1.0), lines * 0.75), base.a);
}
