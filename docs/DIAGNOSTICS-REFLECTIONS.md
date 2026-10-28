# Diagnostics and reflection detail

The existing FPS/draw-stat overlay now defaults on. Explicit `debug_draw = 0`
in `[video]` still hides it, and the existing Advanced graphics option uses the
same default. This does not add profiling panels or unrelated debug clutter.

Existing dynamic car cube maps now default to128 pixels/face instead of the
stage-selected64/128. `reflection_resolution = 64`,128 or256 controls detail;
`0` restores the original stage-selected size. Inputs are bounded to those tiers.
This improves the potential detail of reflections already supported by the
material/render-device mode, not the number of surfaces that reflect. The
existing cube-map render mode still needs to be selected. Reflection texture
sampling is linear in that path. No SSR, ray tracing or new geometry is claimed.

Doubling cube width from64 to128 means4x face pixels and correspondingly more
reflection GPU work/storage, not a free quality improvement.256 is16x64's pixels.
Native game image quality and frame-time cost remain unverified. Use the new FPS
counter to spot gross changes, but actual GPU frame-time needs Metal profiling.
Do not treat a Linux build/test pass as appearance or performance acceptance.

F3 (DIK0x3D / decimal61) triggers one windscreen-wiper sweep; it is not an on/off
toggle. Automatic wipers use `video.windscreen_wipers`. The manual action is
currently gated by nonzero `video.windscreen_dirt` in Race.cpp.

The log's `N glyphs from the bitmap` is static raster font fallback, not animated
characters. The separate scenery animation path is not a guarantee of animated
assets: current code assumes consecutive texture IDs and four frames, and menu
versus render defaults disagree. Blindly enabling that path would risk cycling
unrelated textures. This change does not do that.

Geometry is separate from image reconstruction. Splitting flat triangles only
adds triangles to the same shape. Smoothing/subdivision can change silhouettes
but needs crease/UV/material/damage constraints or it rounds intentional edges
and breaks textures. Inspect real car meshes and authored LODs first; any future
cosmetic geometry work must leave collision and physics unchanged.
