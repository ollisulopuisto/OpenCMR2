#version 450
// Optional atmosphere fog (video.fog): distance fog + ground-mist gradient + sun-ward
// glow. Pure maths reference: render/fog_math.h. One full-screen pass, scene + depth in.
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D scene;
layout(set = 2, binding = 1) uniform sampler2D depthTex;
layout(std140, set = 3, binding = 0) uniform FogUniforms {
    vec4 fog;   // x density per world unit, y glow strength 0..1, z near, w far (0 = projection unknown)
    vec4 sun;   // xy projected sun (uv, v downwards), z sun visible (0/1), w aspect ratio
};
void main()
{
    vec2 size = vec2(textureSize(scene, 0));
    vec2 uv = gl_FragCoord.xy / size;
    vec4 center = texture(scene, uv);
    float depth = texture(depthTex, uv).r;
    float factor = 0.0;
    if (fog.z > 0.0 && fog.w > fog.z) {
        float z = fog.z * fog.w / max(fog.w - depth * (fog.w - fog.z), 1e-6);
        factor = 1.0 - exp(-fog.x * z);
    } else {
        // Unknown projection: ramp over the top slice of the hyperbolic raw depth.
        float t = clamp((depth - 0.990) / 0.010, 0.0, 1.0);
        factor = 1.0 - exp(-fog.x * 150.0 * t * t);
    }
    // Ground mist: 0.55x at the top of the frame, 1.45x at the bottom.
    factor = clamp(factor * (0.55 + 0.9 * smoothstep(0.15, 0.85, uv.y)), 0.0, 1.0);
    // The sky hazes but never turns into a flat wall.
    if (depth >= 0.9999) factor = min(factor, 0.85);
    if (factor <= 0.0) { outColor = center; return; }
    // Sun-ward glow: the fog brightens and warms toward the projected sun.
    vec2 d = uv - sun.xy;
    d.x *= sun.w;
    float g = sun.z > 0.5 ? fog.y * pow(max(1.0 - length(d) * 1.6, 0.0), 3.0) : 0.0;
    vec3 fogColor = mix(vec3(0.62, 0.66, 0.72), vec3(0.98, 0.87, 0.70), clamp(g, 0.0, 1.0));
    fogColor *= 1.0 + g * 0.6;
    outColor = vec4(mix(center.rgb, fogColor, factor), center.a);
}
