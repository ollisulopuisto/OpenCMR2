// The shared touch controls' settled semantics, off device. Every case
// here is a user-confirmed behavior (2026-11-01); tunable thresholds are
// not tested beyond the configured placeholders.

#include "TouchLayout.h"

#include <cmath>
#include <cstdio>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

namespace {

bool Near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

int DrivingCircleSteersAcceleratesBrakes()
{
    TouchControls t;
    t.PointerDown(1, 0.25f, 0.6f); // driving side for a right-handed layout
    t.PointerMove(1, 0.35f, 0.6f);
    CHECK(Near(t.Input().steer, 1.0f));  // full right
    t.PointerMove(1, 0.20f, 0.6f);
    CHECK(Near(t.Input().steer, -0.5f)); // half left
    t.PointerMove(1, 0.25f, 0.5f);
    CHECK(Near(t.Input().steer, 0.0f));
    CHECK(Near(t.Input().accelerate, 1.0f)); // up = accelerate
    CHECK(Near(t.Input().brake, 0.0f));
    t.PointerMove(1, 0.25f, 0.65f);
    CHECK(Near(t.Input().brake, 0.5f));      // down = brake
    CHECK(Near(t.Input().accelerate, 0.0f));
    t.PointerUp(1);
    CHECK(Near(t.Input().steer, 0.0f) && Near(t.Input().brake, 0.0f));
    return 0;
}

int BrakeCircleStates()
{
    TouchControls t;
    t.PointerDown(1, 0.75f, 0.6f); // brake side
    CHECK(Near(t.Input().brake, 0.0f) && !t.Input().handbrake); // neutral at touch-down
    t.PointerMove(1, 0.75f, 0.68f);
    CHECK(t.Input().brake > 0.0f && !t.Input().handbrake);      // down = brake
    t.PointerMove(1, 0.75f, 0.80f);
    CHECK(t.Input().brake > 0.0f && t.Input().handbrake);       // farther = both
    t.PointerMove(1, 0.75f, 0.50f);
    CHECK(Near(t.Input().brake, 0.0f) && t.Input().handbrake);  // up = handbrake
    t.PointerUp(1);
    CHECK(Near(t.Input().brake, 0.0f) && !t.Input().handbrake); // lift releases all
    return 0;
}

int ReversalSwitchesBrakeToHandbrake()
{
    // User-confirmed: reversing direction up out of braking becomes the
    // handbrake without stopping in neutral.
    TouchControls t;
    t.PointerDown(1, 0.75f, 0.6f);
    t.PointerMove(1, 0.75f, 0.70f);           // braking
    CHECK(t.Input().brake > 0.0f && !t.Input().handbrake);
    t.PointerMove(1, 0.75f, 0.605f);          // back through the anchor: still braking, no neutral stop
    CHECK(t.Input().brake > 0.0f && !t.Input().handbrake);
    t.PointerMove(1, 0.75f, 0.54f);           // past the handbrake threshold: handbrake now
    CHECK(Near(t.Input().brake, 0.0f) && t.Input().handbrake);
    return 0;
}

int AggregateBrakeSurvivesOneRelease()
{
    // Settled invariant: both circles brake at once, the inputs add up,
    // and only the last release ends braking.
    TouchControls t;
    t.PointerDown(1, 0.25f, 0.6f);            // driving circle
    t.PointerMove(1, 0.25f, 0.68f);           // braking on the driving circle
    t.PointerDown(2, 0.75f, 0.6f);            // brake circle
    t.PointerMove(2, 0.75f, 0.70f);           // braking there too
    CHECK(t.Input().brake > 0.0f);
    t.PointerUp(1);                            // one source releases
    CHECK(t.Input().brake > 0.0f);             // the other still holds
    t.PointerUp(2);                            // the last release ends braking
    CHECK(Near(t.Input().brake, 0.0f));
    return 0;
}

int AllFourInputsCombine()
{
    TouchControls t;
    t.PointerDown(1, 0.25f, 0.6f);
    t.PointerMove(1, 0.30f, 0.55f);           // steer right + accelerate
    t.PointerDown(2, 0.75f, 0.6f);
    t.PointerMove(2, 0.75f, 0.80f);           // brake + handbrake
    TouchInput in = t.Input();
    CHECK(in.steer > 0.0f && in.accelerate > 0.0f && in.brake > 0.0f && in.handbrake);
    return 0;
}

int HandednessMirrors()
{
    TouchConfig cfg;
    cfg.hand = TOUCH_HAND_LEFT;
    TouchControls t(cfg);
    t.PointerDown(1, 0.75f, 0.6f);            // right side drives for left-handed
    t.PointerMove(1, 0.80f, 0.55f);
    CHECK(t.Input().steer > 0.0f && t.Input().accelerate > 0.0f);
    t.PointerDown(2, 0.25f, 0.6f);            // left side brakes
    t.PointerMove(2, 0.25f, 0.70f);
    CHECK(t.Input().brake > 0.0f);
    return 0;
}

int ClassicModeStaysAvailable()
{
    TouchConfig cfg;
    cfg.mode = TOUCH_MODE_CLASSIC;
    TouchControls t(cfg);
    t.PointerDown(1, 0.25f, 0.8f);            // steering pad
    t.PointerMove(1, 0.30f, 0.8f);
    CHECK(t.Input().steer > 0.0f);
    CHECK(Near(t.Input().accelerate, 0.0f));  // classic pad steers only
    t.PointerDown(2, 0.75f, 0.2f);            // accelerate button
    CHECK(Near(t.Input().accelerate, 1.0f));
    t.PointerDown(3, 0.8f, 0.55f);            // brake button
    CHECK(Near(t.Input().brake, 1.0f));
    t.PointerUp(2);                            // the brake button still holds
    CHECK(Near(t.Input().accelerate, 0.0f) && Near(t.Input().brake, 1.0f));
    t.PointerDown(4, 0.58f, 0.55f);           // handbrake button
    CHECK(t.Input().handbrake);
    return 0;
}

} // namespace

int main()
{
    if (int r = DrivingCircleSteersAcceleratesBrakes()) return r;
    if (int r = BrakeCircleStates()) return r;
    if (int r = ReversalSwitchesBrakeToHandbrake()) return r;
    if (int r = AggregateBrakeSurvivesOneRelease()) return r;
    if (int r = AllFourInputsCombine()) return r;
    if (int r = HandednessMirrors()) return r;
    if (int r = ClassicModeStaysAvailable()) return r;
    printf("touch layout ok\n");
    return 0;
}
