#pragma once
// F10 view cycle. 0 = full debug info (the start), 1 = debug info and the adjustment panel, 2 = nothing, 3 = frame rate only.
inline int TuningMode_Next(int mode) { return (mode + 1) % 4; }
inline bool TuningMode_AdjustShown(int mode) { return mode == 1; }
inline bool TuningMode_InfoShown(int mode) { return mode != 2; }      // any debug text (the full panels or the frame rate)
inline bool TuningMode_FpsOnly(int mode) { return mode == 3; }
inline bool TuningMode_FullInfo(int mode) { return mode == 0 || mode == 1; }
