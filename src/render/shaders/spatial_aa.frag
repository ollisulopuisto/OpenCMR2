#version 450
// OpenCMR2: opt-in, single-frame edge smoothing. No history or motion vectors.
// Run before bloom/grain so their detail does not drive the edge detector.
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D scene;
layout(std140, set = 3, binding = 0) uniform SpatialAAUniforms {
    vec4 size; // reciprocal width/height, unused, unused
};
float luma(vec3 rgb) { return dot(rgb, vec3(0.299, 0.587, 0.114)); }
void main()
{
    vec2 uv = gl_FragCoord.xy * size.xy;
    vec4 center = texture(scene, uv);
    float m = luma(center.rgb);
    float nw = luma(texture(scene, uv + vec2(-1, -1) * size.xy).rgb);
    float ne = luma(texture(scene, uv + vec2( 1, -1) * size.xy).rgb);
    float sw = luma(texture(scene, uv + vec2(-1,  1) * size.xy).rgb);
    float se = luma(texture(scene, uv + vec2( 1,  1) * size.xy).rgb);
    float lo = min(m, min(min(nw, ne), min(sw, se)));
    float hi = max(m, max(max(nw, ne), max(sw, se)));
    // Keep flat areas and low-contrast texture detail untouched.
    if (hi - lo < max(0.03125, hi * 0.125)) {
        outColor = center;
        return;
    }
    vec2 direction = vec2(-((nw + ne) - (sw + se)), (nw + sw) - (ne + se));
    float reduce = max((nw + ne + sw + se) * 0.03125, 0.0078125);
    direction = clamp(direction / (min(abs(direction.x), abs(direction.y)) + reduce),
                      vec2(-8), vec2(8)) * size.xy;
    vec3 inner = 0.5 * (texture(scene, uv - direction / 6.0).rgb +
                        texture(scene, uv + direction / 6.0).rgb);
    vec3 outer = inner * 0.5 + 0.25 * (texture(scene, uv - direction * 0.5).rgb +
                                       texture(scene, uv + direction * 0.5).rgb);
    float value = luma(outer);
    outColor = vec4(value < lo || value > hi ? inner : outer, center.a);
}
