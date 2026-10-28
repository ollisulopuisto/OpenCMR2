# Temporal reconstruction: first implementation slice

This PR adds CPU-side history and motion-math building blocks, not a temporal
renderer. It has no runtime setting, does not change the camera, does not
allocate GPU history and does not call MetalFX temporal APIs. Spatial MetalFX
remains separate and opt-in. All default game rendering stays unchanged.

## Implemented and tested

- Eight-sample Halton(2,3) pixel offsets, bounded to half a pixel.
- Current-to-previous motion from unjittered row-vector clip transforms,
  in input-resolution pixels with a top-left origin.
- Explicit object identity, never draw-order matching or a vertex-buffer key.
  Mesh buffers are shared by instances, so those shortcuts would confuse cars
  or placed scenery when culling changes.
- Successful-frame-only history commits, invalidation on nonconsecutive frame
  numbers, input/output size changes, camera identity and scene identity.
- Explicit reset for camera cuts, seeks, stage changes, pause/resume and device
  loss. Duplicate IDs and nonfinite transforms fail closed. Reappearing objects
  cannot reuse transforms from frames in which they were not drawn.

The CPU tests cover the state machine and signed motion convention. They do not
prove a GPU velocity buffer, MetalFX output, real gameplay quality or frame time.

## Remaining integration work, in dependency order

1. Add stable instance IDs at the game mesh draw boundary. Sector mesh identity
   and placed object identity must be distinct from their shared Mesh pointer.
   Car parts and animated/deformed vertices require their own previous state.
2. Capture per-instance previous world/view/projection and previous deformation.
   Split HUD/transformed draws, menus, mirrors and cube-map draws from the main
   scene. Existing Execute calls can flush partial frames without presenting;
   those calls must not advance temporal history or jitter.
3. Add a scene velocity target (RG16F) and sampleable depth with format checks.
   Match opaque/alpha-tested coverage and depth exactly. Mark unreliable motion
   and transparency conservatively rather than inventing zero motion.
4. Jitter only the scene projection. The offset generator here is a sample
   displacement; the scaler's jitter sign must be verified against the actual
   raster coordinate convention. Use unjittered transforms for velocity.
5. Extend the pinned SDL Metal backend with a separate temporal callback. Keep
   native command buffers/textures and in-flight resource tracking inside SDL,
   as with the spatial path. Check hardware, formats and texture usages, cycle
   output, and fall back normally on any failure. Never cast private SDL
   resources in application code.
6. Reconstruct before HUD/post-effects composition, reset history at explicit
   gameplay transitions, and commit only on successful encoding/submission.
7. Compile on Mac, then inspect actual GPU output: static fine lines, lateral
   motion, moving cars, disocclusion, camera cuts, resize, menus, rain and HUD.
   Measure frame time separately. A green CPU test is not this acceptance.

Apple's reference defines current-to-previous render-pixel motion, reset on
first frame/cuts, and post-reconstruction UI:
https://developer.apple.com/videos/play/wwdc2022/10103/
