#version 450
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D sceneTex;
layout(set = 2, binding = 1) uniform sampler2D depthTex;
layout(std140, set = 3, binding = 0) uniform DoFUniforms {
    float focalDistance;
    float focalRange;
    float maxBlur;
    float tunnel;  // 0..1 speed-driven tunnel vision (0 = off, identical to the old pass)
    vec4 extra;    // x aspect ratio, y peripheral (tunnel) blur radius in pixels at full tunnel
};
void main()
{
    vec2 pixel = 1.0 / vec2(textureSize(sceneTex, 0));
    vec2 uv = gl_FragCoord.xy * pixel;
    vec4 center = texture(sceneTex, uv);
    float depth = texture(depthTex, uv).r;
    float coc = smoothstep(0.0, max(focalRange, 0.001), abs(depth - focalDistance));
    float tunnelRadius = 0.0;
    // Tunnel vision: blur grows with distance from the screen centre, and the sharp circle
    // closes as speed rises. The car sits low in the frame (r about 0.5) and stays sharp.
    if (tunnel > 0.0) {
        vec2 centered = (uv - 0.5) * 2.0;
        centered.x *= extra.x;
        float edge = length(centered);
        float inner = mix(1.4, 0.7, tunnel);
        float peripheral = smoothstep(inner, inner + 0.8, edge) * tunnel;
        coc = max(coc, peripheral);
        tunnelRadius = peripheral * extra.y;
    }
    // The peripheral blur has its own radius: dof_blur (about 1 px at the default) is far too small to see.
    float radius = max(coc * max(maxBlur, 0.0), tunnelRadius);
    if (radius < 0.25) { outColor = center; return; }
    // Golden-angle disk: smoother defocus than a box, with foreground rejection
    // to avoid pulling a sharp foreground silhouette into the blurred distance.
    vec3 sum = center.rgb;
    float weight = 1.0;
    for (int i = 0; i < 16; ++i) {
        float angle = float(i) * 2.39996323;
        float r = sqrt((float(i) + 0.5) / 16.0) * radius;
        vec2 pos = uv + vec2(cos(angle), sin(angle)) * r * pixel;
        float sampleDepth = texture(depthTex, pos).r;
        float w = 1.0 - smoothstep(0.002, 0.02, depth - sampleDepth);
        sum += texture(sceneTex, pos).rgb * w;
        weight += w;
    }
    outColor = vec4(sum / weight, center.a);
}
