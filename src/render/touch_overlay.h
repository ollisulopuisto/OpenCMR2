/*
 * Visible on-screen touch pads. Draws the shared touch layer's pad geometry
 * (src/input/TouchLayout.h) as generated ring + thumb-dot sprites, recorded
 * at the end of race frames from Gfx_Present (src/render/gfx_core.cpp), so
 * the pads ride the normal command stream on every backend.
 *
 * All sizes and colours here are placeholder visuals: the settled design
 * owns the final look (see TouchLayout.h's unsettled list). The texture
 * generators are pure functions so the shapes are unit-testable off device.
 */
#ifndef OPENCMR2_TOUCH_OVERLAY_H
#define OPENCMR2_TOUCH_OVERLAY_H

// Ring: transparent inside and outside, a soft opaque band at the rim.
// Dot: soft-edged filled disc. Both write BGRA white pixels; alpha carries
// the shape and vertex colour modulates the draw.
void TouchOverlay_FillRingBGRA(unsigned char *out, int size);
void TouchOverlay_FillDotBGRA(unsigned char *out, int size);

// Records this frame's pads into the command stream. Called at the top of
// Present() while the frame's race-world flag still says whether this was a
// driving frame; no-op unless the shared touch layer is active.
void TouchOverlay_RecordFrame();

#endif // OPENCMR2_TOUCH_OVERLAY_H
