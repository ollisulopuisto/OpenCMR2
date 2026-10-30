#pragma once
// Which views show the speed-driven screen effects (tunnel vision, its lines, the radial streak). Visual only.
// World-space vapor trails and dust are not covered here and stay as set.
//  - Replay playback: only the interior view (camera type 3) keeps them; every other view drops them.
//  - Drone camera (the camera operator's variant of chase type 5), driving or replay: always off. The pilot does not feel speed that way.
// The user's settings are never rewritten: the effect is only held at zero while the view is active, so other views restore.
inline bool SpeedLook_ExternalReplayView(int viewType, bool replayPlaying)
{
    if (!replayPlaying) return false;
    return viewType != 3;
}
// Replay playback, every view: no vapor trails (air vortices). Driving keeps them. The saved video.vapor_trails value is never touched.
inline bool SpeedLook_VaporOff(bool replayPlaying) { return replayPlaying; }
// speedEffectsOn is the graphics-menu switch (video.speed_effects, default on). Off holds the effects at zero in every view; the setting is not rewritten.
inline bool SpeedLook_ScreenEffectsOff(int viewType, bool replayPlaying, bool droneView, bool speedEffectsOn = true)
{
    return !speedEffectsOn || droneView || SpeedLook_ExternalReplayView(viewType, replayPlaying);
}
