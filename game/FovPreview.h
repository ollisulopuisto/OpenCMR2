#ifndef OPENCMR2_FOV_PREVIEW_H
#define OPENCMR2_FOV_PREVIEW_H
// OpenCMR2 debug (visual only): a live override of video.fov, so the same camera can be looked at with FOV 25, 40, 75, 90 ...
// video.fov is the SilentPatch-style number: the projection zoom is scaled by 70 / fov, and a higher number is a wider view.
// It is an ABSOLUTE setting, not the Speed FOV %, which widens the view relative to it as the car speeds up. The preview
// replaces only the absolute number, 0 = off (the saved video.fov is used), and it is never saved by itself.
inline int FovPreview_Effective(int preview, int setting)
{
    if (preview <= 0) return setting < 30 ? 30 : setting > 150 ? 150 : setting;   // the old clamp for the saved setting
    return preview < 20 ? 20 : preview > 150 ? 150 : preview;                     // the preview may go a little narrower
}
// Zoom scale for an effective fov, the same formula the game used before (0x275c2 is the original's value at 70).
inline double FovPreview_Scale(int fov) { return 0x275c2 * 70.0 / fov; }
// How much bigger things look than at the saved setting (1.0 = same, below 1 = wider view).
inline double FovPreview_Magnification(int effective, int setting) { return (double)setting / effective; }
#endif
