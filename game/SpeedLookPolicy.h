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
inline bool SpeedLook_ScreenEffectsOff(int viewType, bool replayPlaying, bool droneView)
{
    return droneView || SpeedLook_ExternalReplayView(viewType, replayPlaying);
}
