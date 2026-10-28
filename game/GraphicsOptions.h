#ifndef _GRAPHICS_OPTIONS_H
#define _GRAPHICS_OPTIONS_H

// OpenCMR2: pure helpers behind the advanced graphics page's rows: the user
// draw distance ([video] video.draw_distance) and the texture filtering level
// ([video] video.anisotropy). Both are saved in opencmr2.ini, not in the save
// game's graphics bits. Kept free of game and renderer includes so the tests
// can exercise the mappings alone.

#include "FixedPoint.h"
#include <cstdint>

struct StageObject;

// The user draw distance as a percent of the stage's far plane, clamped to a
// sane range so the far plane can't be pushed to a useless extreme (a quarter
// of the stage distance up to four times it). 100 is the original distance.
inline int View_ClampDrawDistancePercent(int percent)
{
    return percent < 25 ? 25 : percent > 400 ? 400 : percent;
}

// The draw-distance steps the menu offers, in percent.
#define DRAW_DISTANCE_COUNT 7
inline const int *DrawDistance_Percents(void)
{
    static const int percents[DRAW_DISTANCE_COUNT] = { 25, 50, 75, 100, 125, 150, 200 };
    return percents;
}

// The menu index of a percent (the nearest step offered; the setting itself
// may hold any value) and the percent of an index.
inline int DrawDistance_PercentToIndex(int percent)
{
    const int *percents = DrawDistance_Percents();
    int best = 0;
    int bestDiff = percent - percents[0];
    if (bestDiff < 0)
        bestDiff = -bestDiff;
    for (int i = 1; i < DRAW_DISTANCE_COUNT; i++) {
        int diff = percent - percents[i];
        if (diff < 0)
            diff = -diff;
        if (diff < bestDiff) {
            bestDiff = diff;
            best = i;
        }
    }
    return best;
}

inline int DrawDistance_IndexToPercent(int index)
{
    if (index < 0)
        index = 0;
    if (index > DRAW_DISTANCE_COUNT - 1)
        index = DRAW_DISTANCE_COUNT - 1;
    return DrawDistance_Percents()[index];
}

// The anisotropic filtering levels the menu offers; 1 is off, the original's
// point sampling without anisotropy.
#define ANISOTROPY_LEVEL_COUNT 5
inline const int *Anisotropy_Levels(void)
{
    static const int levels[ANISOTROPY_LEVEL_COUNT] = { 1, 2, 4, 8, 16 };
    return levels;
}

// The menu index of a level (the nearest offered) and the level of an index.
inline int Anisotropy_LevelToIndex(int level)
{
    const int *levels = Anisotropy_Levels();
    int best = 0;
    int bestDiff = level - levels[0];
    if (bestDiff < 0)
        bestDiff = -bestDiff;
    for (int i = 1; i < ANISOTROPY_LEVEL_COUNT; i++) {
        int diff = level - levels[i];
        if (diff < 0)
            diff = -diff;
        if (diff < bestDiff) {
            bestDiff = diff;
            best = i;
        }
    }
    return best;
}

inline int Anisotropy_IndexToLevel(int index)
{
    if (index < 0)
        index = 0;
    if (index > ANISOTROPY_LEVEL_COUNT - 1)
        index = ANISOTROPY_LEVEL_COUNT - 1;
    return Anisotropy_Levels()[index];
}

// OpenCMR2: runtime-only flag bit in StageObject::flags marking a tree smashed
// by the car. Bit 28 (0x10000000) is unused by the original's flag masks
// (0x8000, 0x4000, 0x2001000). Stages reload with fresh trees, so the bit
// need not be part of the on-disk record semantics.
#define STAGE_OBJECT_BROKEN 0x10000000u

// OpenCMR2: whether a hit at the given car speed (16.16) smashes a breakable
// tree. Pure: no renderer or game state; the caller gates on
// [video] video.tree_break. Below the threshold the tree is solid and the
// crash stops the car; at or above it the tree is smashed and the car drives
// through. Tunable constant.
inline bool TreeBreak_ShouldBreak(int speed)
{
    return speed >= 0x20000;
}

// OpenCMR2: dirt on the camera lens. A level of 0 is a clean lens; 255 is
// completely obscured. Impacts and off-road driving add dirt, each frame
// decays it. Saved nowhere: a fresh stage starts clean.
// [video] video.camera_effects gates the whole system.
#define LENS_DIRT_MAX 255u

// Adds `amount` of dirt to `level`, clamping at LENS_DIRT_MAX.
inline unsigned int LensDirt_Accumulate(unsigned int level, int amount)
{
    unsigned int sum = level + (unsigned int)amount;
    return sum > LENS_DIRT_MAX ? LENS_DIRT_MAX : sum;
}

// Decays `level` by `amount` per frame, flooring at 0.
inline unsigned int LensDirt_Decay(unsigned int level, int amount)
{
    return level < (unsigned int)amount ? 0 : level - (unsigned int)amount;
}

// Whether the dirt level is worth drawing (any non-zero level shows).
inline bool LensDirt_ShouldDraw(unsigned int level)
{
    return level != 0;
}

// Per-view dirt-on-the-lens levels, defined in RallyData.cpp.
extern unsigned int g_lensDirt[8];

// OpenCMR2: human-operator camera shake. Amplitude grows with car speed and
// impact strength, clamped so the camera never leaves the track. The shake is
// applied as a per-frame random jitter to the chase-camera position.
// [video] video.camera_effects gates it.
#define SHAKE_MAX_AMPLITUDE 0x30000

// Shake amplitude (16.16) from speed and impact level (0..100).
inline int Shake_Amplitude(int speed, int impact)
{
    int amp = FixMul(speed, 0x1999) + FixMul(impact, 0x1999);
    return amp > SHAKE_MAX_AMPLITUDE ? SHAKE_MAX_AMPLITUDE : amp;
}

// OpenCMR2: lens flare brightness from sun visibility (0..100).
#define FLARE_MAX_BRIGHTNESS 0xff

// Flare brightness (0..0xff) from sun visibility, off below a faint threshold.
inline int Flare_Brightness(int sunVisibility)
{
    if (sunVisibility < 10)
        return 0;
    return (sunVisibility * FLARE_MAX_BRIGHTNESS) / 100;
}

// OpenCMR2: animated scenery idle cycles (crowds, trees). A runtime-only flag
// bit in StageObject::flags marks an object as animated; per-object animation
// state lives in a global table keyed by the StageObject pointer (the draw
// path reinterprets the same bytes as a world matrix, so it cannot share the
// object's unused tail). [video] video.animated_scenery gates the whole system.
#define STAGE_OBJECT_ANIMATED 0x40000000u

// Per-object animation state. Kept in a table, not on the object.
struct AnimState {
    struct StageObject *pObject;  // key: the stage object this state belongs to
    int baseTexture;   // first texture of the cycle
    uint8_t frame;     // current frame (0..frameCount-1)
    uint8_t frameCount;// frames in the cycle
    uint8_t timer;     // ticks until the next frame advance
    uint8_t initialized;// 1 once baseTexture/frameCount are set
};

// Table of per-object animation state, defined in Game.cpp. Cleared when a
// stage is unloaded so a fresh stage starts its objects still.
extern struct AnimState *g_animStates;
extern unsigned int g_animStateCount;
void AnimObject_ClearTable(void);

// Advances the animation frame by one tick: decrements the timer, and when it
// reaches zero, wraps the frame within frameCount and resets the timer.
inline int AnimFrame_Advance(int frame, int frameCount, int *timer)
{
    (*timer)--;
    if (*timer <= 0) {
        *timer = 5; // ticks per frame (about 1/12 s at 60 fps)
        return (frame + 1) % frameCount;
    }
    return frame;
}

// Animated texture index for an object: base texture + current frame.
inline int AnimTexture_Index(int baseTexture, int frame)
{
    return baseTexture + frame;
}

#endif
