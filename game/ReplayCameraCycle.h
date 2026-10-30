#pragma once
// Replay camera order (Tab): 7 automatic trackside, 4 chase, 5 chase/drone, 1 car-mounted, 2 bonnet, 3 cockpit, 11 free camera, then 7 again.
// `available` has bit m set when mode m can be selected right now (View_IsModeAvailable plus the driver-mode rules). A mode that cannot be
// selected is skipped, because View_SwitchCamera silently refuses it. A current mode outside the list (6, 8, 9, 10) starts from the top.
// Returns -1 when nothing else is available.
static const int kReplayFreeCamMode = 11;   // not a game camera type: the free camera is a pose override on top of chase 4
static const int kReplayCameraOrder[7] = {7, 4, 5, 1, 2, 3, kReplayFreeCamMode};
inline int ReplayCamera_Next(int current, unsigned available)
{
    int at = -1;
    for (int i = 0; i < 7; i++) if (kReplayCameraOrder[i] == current) at = i;
    for (int step = 1; step <= 7; step++) {
        int mode = kReplayCameraOrder[(at + step + 7) % 7];
        if (mode == current) continue;
        if (available & (1u << mode)) return mode;
    }
    return -1;
}
