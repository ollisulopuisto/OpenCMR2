#ifndef OPENCMR2_TUNING_PANEL_H
#define OPENCMR2_TUNING_PANEL_H
// Live, in-race tuning of the visual effects (no physics). Pure row maths; the key handling and drawing live in RallyData.cpp.
struct TuningRow { const char *label, *key; int lo, hi, step, fallback; };
inline const TuningRow *Tuning_Rows(int &count)
{
    static const TuningRow rows[] = {
        {"Speed FOV %", "video.speed_fov", 0, 40, 2, 0},
        {"Speed motes (0/1)", "video.speed_motes", 0, 1, 1, 0},
        {"Mote amount %", "video.speed_motes_amount", 0, 300, 25, 100},
        {"Mote stretch", "video.speed_motes_stretch", 0, 100, 10, 0},
        {"DoF (0/1)", "video.depth_of_field", 0, 1, 1, 0},
        {"DoF blur %", "video.dof_blur", 0, 100, 10, 30},
        {"DoF max px at 100%", "video.dof_blur_max_px", 1, 40, 2, 4},
        {"Tunnel vision %", "video.tunnel_vision", 0, 100, 10, 100},
        {"Motion blur (0/1)", "video.motion_blur", 0, 1, 1, 1},
        {"Motion blur %", "video.motion_blur_amount", 0, 100, 10, 16},
    };
    count = (int)(sizeof(rows) / sizeof(*rows));
    return rows;
}
// One step in direction dir (+1/-1), clamped to the row range. A value already outside the range snaps in.
inline int Tuning_Step(const TuningRow &row, int value, int dir)
{
    long v = (long)value + (long)dir * row.step;
    if (v < row.lo) v = row.lo;
    if (v > row.hi) v = row.hi;
    return (int)v;
}
// Row selection wraps around.
inline int Tuning_Select(int index, int dir, int count) { return count <= 0 ? 0 : ((index + dir) % count + count) % count; }
// Physical-key scancodes (DirectInput numbering, the same on every keyboard layout): the digit row 1..7 and F10.
// The old [ ] keys were the default gear shifts, so they are not used. While the panel is open, its keys are removed from
// the keyboard state the game reads, so they never also drive the car or a menu.
static const int kTuningToggleDefault = 0x44;                       // F10
static const int kTuningKeys[7] = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}; // 1 prev row, 2 next row, 3 down, 4 up, 5 reset row, 6 reset all, 7 save
inline void Tuning_FilterKeys(unsigned char keys[256], bool open, int toggleKey)
{
    if (toggleKey > 0 && toggleKey < 256) keys[toggleKey] = 0;
    if (open)
        for (int k : kTuningKeys) keys[k] = 0;
}
// Edge detector: true once per press (key went from up to down), never while held.
struct TuningEdges {
    bool prev[8] = {};
    bool Edge(int slot, bool down) { bool e = down && !prev[slot]; prev[slot] = down; return e; }
};
#endif
