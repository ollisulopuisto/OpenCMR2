#version 450
// OpenCMR2: opt-in temporal anti-aliasing resolve (video.temporal_aa).
// PARTIAL STAGE: camera-matrix reprojection from the depth buffer plus a 3x3
// neighbourhood clamp. There is no per-object velocity, so moving objects rely
// on the clamp and can still smear. This is not MetalFX temporal scaling.
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D scene;
layout(set = 2, binding = 1) uniform sampler2D depthTex;
layout(set = 2, binding = 2) uniform sampler2D history;
layout(std140, set = 3, binding = 0) uniform TemporalUniforms {
    mat4 inverseViewProjection;
    mat4 previousViewProjection;
    vec4 params; // x history weight (0 = no valid history), y clamp slack
    vec4 target; // reciprocal width, reciprocal height
};
void main()
{
    vec2 uv = gl_FragCoord.xy * target.xy;
    vec3 c = texture(scene, uv).rgb;
    if (params.x <= 0.0) { outColor = vec4(c, 1.0); return; }
    vec3 lo = c;
    vec3 hi = c;
    vec3 m1 = vec3(0.0);
    vec3 m2 = vec3(0.0);
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec3 s = texture(scene, uv + vec2(float(x), float(y)) * target.xy).rgb;
            lo = min(lo, s);
            hi = max(hi, s);
            m1 += s;
            m2 += s * s;
        }
    }
    float depth = texture(depthTex, uv).r;
    vec4 p = inverseViewProjection * vec4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    vec4 q = previousViewProjection * vec4(p.xyz / p.w, 1.0);
    if (q.w <= 0.0) { outColor = vec4(c, 1.0); return; }
    vec2 n = q.xy / q.w;
    vec2 previousUv = vec2(n.x * 0.5 + 0.5, 0.5 - n.y * 0.5);
    if (previousUv.x < 0.0 || previousUv.y < 0.0 || previousUv.x > 1.0 || previousUv.y > 1.0) {
        outColor = vec4(c, 1.0);
        return;
    }
    vec3 h = texture(history, previousUv).rgb;
    // Variance clip: tighter than the min/max box, so moving objects (no velocity yet)
    // reject stale history sooner. The box is intersected with min/max.
    vec3 mean = m1 / 9.0;
    vec3 sigma = sqrt(max(m2 / 9.0 - mean * mean, vec3(0.0)));
    vec3 vlo = max(lo, mean - 1.5 * sigma) - vec3(params.y);
    vec3 vhi = min(hi, mean + 1.5 * sigma) + vec3(params.y);
    vec3 hc = clamp(h, vlo, vhi);
    // The further history had to be clamped, the less it is trusted.
    float rejected = clamp(length(h - hc) * 4.0, 0.0, 1.0);
    outColor = vec4(mix(c, hc, params.x * (1.0 - rejected)), 1.0);
}
