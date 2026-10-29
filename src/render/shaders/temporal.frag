#version 450
// OpenCMR2: opt-in temporal anti-aliasing resolve (video.temporal_aa).
// PARTIAL STAGE: camera-matrix reprojection from the depth buffer plus a 3x3
// neighbourhood clamp. Cars get a rigid-translation velocity (no rotation); everything else
// moving relies on the clamp and can still smear. This is not MetalFX temporal scaling.
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D scene;
layout(set = 2, binding = 1) uniform sampler2D depthTex;
layout(set = 2, binding = 2) uniform sampler2D history;
layout(set = 2, binding = 3) uniform sampler2D previousDepthTex;
layout(std140, set = 3, binding = 0) uniform TemporalUniforms {
    mat4 inverseViewProjection;
    mat4 previousViewProjection;
    vec4 params; // x history weight (0 = no valid history), y clamp slack, z relative view-distance tolerance, w projection A (ndc = A + B/viewZ)
    vec4 target; // reciprocal width, reciprocal height, projection B
    vec4 carCenter[8]; // xyz world position, w gating radius (0 = unused)
    vec4 carDelta[8];  // xyz translation since last frame, w 1 = has history
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
    vec3 worldPos = p.xyz / p.w;
    // Candidate 1: static world (camera motion only). Candidate 2: the pixel belongs to a car
    // that translated by carDelta. Inside a car's gating sphere both are fetched and the one
    // whose history is closer to the current pixel wins, so background/road inside the sphere
    // keeps its static history instead of being dragged along. Rotation is not reprojected.
    vec3 h = vec3(0.0);
    float bestError = 1e9;
    bool found = false;
    vec3 candidates[2];
    candidates[0] = worldPos;
    candidates[1] = worldPos;
    bool hasCar = false;
    for (int i = 0; i < 8; ++i) {
        if (carCenter[i].w > 0.0 && carDelta[i].w > 0.5 &&
            distance(worldPos, carCenter[i].xyz) < carCenter[i].w) {
            candidates[1] = worldPos - carDelta[i].xyz;
            hasCar = true;
            break;
        }
    }
    for (int k = 0; k < 2; ++k) {
        if (k == 1 && !hasCar) break;
        vec4 q = previousViewProjection * vec4(candidates[k], 1.0);
        if (q.w <= 0.0) continue;
        vec2 n = q.xy / q.w;
        vec2 puv = vec2(n.x * 0.5 + 0.5, 0.5 - n.y * 0.5);
        if (puv.x < 0.0 || puv.y < 0.0 || puv.x > 1.0 || puv.y > 1.0) continue;
        // Disocclusion: the history pixel must have been at the view-space distance this
        // world point had in the previous frame. Depth is linearised first (viewZ = B / (ndc - A)),
        // raw non-linear depth would accept almost anything in the distance.
        float expectedZ = target.z / (q.z / q.w - params.w);
        float storedZ = target.z / (texture(previousDepthTex, puv).r - params.w);
        if (!(expectedZ > 0.0) || !(storedZ > 0.0) ||
            abs(expectedZ - storedZ) > params.z * max(expectedZ, storedZ)) continue;
        vec3 hk = texture(history, puv).rgb;
        float err = dot(hk - c, hk - c);
        if (err < bestError) { bestError = err; h = hk; found = true; }
    }
    if (!found) { outColor = vec4(c, 1.0); return; }
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
