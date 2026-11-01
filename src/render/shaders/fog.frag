#version 450
// Optional atmosphere fog (video.fog): distance fog + ground-mist gradient + sun-ward
// glow + car-light response (video.fog_lights). Pure maths reference: render/fog_math.h.
// One full-screen pass, scene + depth in.
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D scene;
layout(set = 2, binding = 1) uniform sampler2D depthTex;
layout(std140, set = 3, binding = 0) uniform FogUniforms {
    vec4 fog;   // x density per world unit, y glow strength 0..1, z near, w far (0 = projection unknown)
    vec4 sun;   // xy projected sun (uv, v downwards), z sun visible (0/1), w aspect ratio
    vec4 extra; // x near-field clear distance in world units (0 = off), y car-light response 0..2, zw unused
    vec4 lights[8];    // xy projected car light (uv, v downwards), z intensity 0..1, w unused
    vec4 lightInfo[8]; // x depth-buffer value 1 world unit in front of the lamp (<= 0: no depth gate), yzw tint
    vec4 beams[8];     // xy aspect-corrected unit screen beam direction, z wedge length, w unused
    vec4 halos[8];     // xy glow anchor (a few units forward along the beam), z halo depth ref (<= 0: no gate), w unused
};
void main()
{
    vec2 size = vec2(textureSize(scene, 0));
    vec2 uv = gl_FragCoord.xy / size;
    vec4 center = texture(scene, uv);
    float depth = texture(depthTex, uv).r;
    float viewZ = 0.0;
    if (fog.z > 0.0 && fog.w > fog.z) {
        viewZ = fog.z * fog.w / max(fog.w - depth * (fog.w - fog.z), 1e-6);
    } else {
        // Unknown projection: ramp over the top slice of the hyperbolic raw depth.
        float t = clamp((depth - 0.990) / 0.010, 0.0, 1.0);
        viewZ = 150.0 * t * t;
    }
    float factor = 1.0 - exp(-fog.x * viewZ);
    // Near-field clear distance (video.fog_clear): 0 at the camera, full curve beyond.
    if (extra.x > 0.0) {
        float ct = clamp(viewZ / extra.x, 0.0, 1.0);
        factor *= ct * ct * (3.0 - 2.0 * ct);
    }
    // Ground mist: 0.55x at the top of the frame, 1.45x at the bottom.
    factor = clamp(factor * (0.55 + 0.9 * smoothstep(0.15, 0.85, uv.y)), 0.0, 1.0);
    // The sky hazes but never turns into a flat wall.
    if (depth >= 0.9999) factor = min(factor, 0.85);
    // Near-clean air still takes the beams (they carry their own floor below).
    if (factor <= 0.0 && extra.y <= 0.0) { outColor = center; return; }
    // Sun-ward glow: the fog brightens and warms toward the projected sun.
    vec2 d = uv - sun.xy;
    d.x *= sun.w;
    float g = sun.z > 0.5 ? fog.y * pow(max(1.0 - length(d) * 1.6, 0.0), 3.0) : 0.0;
    vec3 fogColor = mix(vec3(0.62, 0.66, 0.72), vec3(0.98, 0.87, 0.70), clamp(g, 0.0, 1.0));
    fogColor *= 1.0 + g * 0.6;
    // Car lights cutting through the fog (video.fog_lights): a screen-space wedge
    // from each lamp along its projected beam direction - an approximation of the
    // air the beam lights, not world-space road illumination. Brightest at the
    // lamps and decaying along the beam (Fog_LightBeam), so the stretch between
    // the lamps and the horizon lights up instead of only the far fog. Fog makes
    // the beam brighter but is not required for it (a 30% floor keeps the beam on
    // near-clean air, e.g. the near road or the cockpit windscreen region). Where
    // the scene sits in front of the light (the car's own body, the dashboard) the
    // depth gate keeps the glow off it; the raw-depth fallback path and the GL
    // renderer pass no per-light depth and keep the glow unoccluded. The soft
    // shoulder and the headroom term keep an already bright horizon from clipping.
    vec3 lightSum = vec3(0.0);
    if (extra.y > 0.0) {
        for (int i = 0; i < 8; i++) {
            float li = lights[i].z;
            if (li <= 0.0) continue;
            vec2 dl = uv - lights[i].xy;
            dl.x *= sun.w;
            vec2 bdir = beams[i].xy;
            float blen = max(beams[i].z, 0.05);
            float wedge = 0.0;
            if (dot(bdir, bdir) > 1e-6) {
                float a = dot(dl, bdir);
                float l = dl.x * bdir.y - dl.y * bdir.x;
                float width = 0.015 + max(a, 0.0) * 0.35;
                float lat = clamp(l / width, -1.0, 1.0);
                float lateral = 1.0 - lat * lat;
                // Short ramp at the lamp avoids a hard edge; zero behind the
                // lamp - no backward shine. The wedge owns the whole beam now
                // that the glow bubble is gone (it used to own the first 0.05).
                float axial = a <= 0.0 ? 0.0 : min(a / 0.01, 1.0);
                wedge = axial * lateral * lateral / (1.0 + 6.0 * max(a, 0.0) / blen);
            }
            float gate = 1.0;
            if (lightInfo[i].x > 0.0)
                gate = step(lightInfo[i].x, depth);
            // Beams only: the glow bubble was removed (user steering,
            // 2026-11-01 - "keep just the beams"). halos[] stays in the
            // uniform layout, pinned to the lamp CPU-side, but is unread here.
            lightSum += lightInfo[i].yzw * (li * wedge * gate);
        }
        lightSum *= extra.y * mix(0.30, 1.0, factor);
    }
    vec3 fogged = mix(center.rgb, fogColor, factor);
    lightSum = vec3(1.0) - exp(-lightSum * 2.0);   // Fog_SoftShoulder
    float head = clamp((1.0 - max(fogged.r, max(fogged.g, fogged.b))) * 1.5, 0.0, 1.0);   // Fog_LightHeadroom
    outColor = vec4(fogged + lightSum * head, center.a);
}
