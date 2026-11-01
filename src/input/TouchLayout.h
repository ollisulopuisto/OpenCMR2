/*
 * Shared touch controls: one implementation for every SDL platform
 * (Android first, iPhone/iPad and a browser build later). A platform layer
 * feeds pointer positions in normalized window coordinates (SDL finger
 * coordinates, y down) and reads the aggregated driving inputs; rendering
 * reads the pad geometry to draw. No SDL or game dependencies, so the
 * behavior is unit-testable off device.
 *
 * Two modes:
 *  - TOUCH_MODE_FLOATING (the new design): two independent floating
 *    circles that anchor where the thumb lands. The driving circle steers
 *    left/right, accelerates up and brakes down. The brake circle is
 *    neutral at touch-down: pull down to brake, farther down to brake and
 *    handbrake together, push up for the handbrake. Reversing up out of
 *    braking crosses to the handbrake without stopping in neutral.
 *  - TOUCH_MODE_CLASSIC: fixed pads mirroring the traditional layout
 *    (steering pad, accelerate/brake buttons, handbrake button), retained
 *    as a selectable alternative.
 *
 * Settled semantics (user, 2026-11-01):
 *  - brake is available on BOTH circles and the brake inputs aggregate:
 *    releasing one pointer never cancels another pointer's brake (only the
 *    last release ends braking);
 *  - direction reversal switches brake to handbrake without a neutral
 *    stop in between;
 *  - all four inputs (steer, accelerate, brake, handbrake) combine freely;
 *  - the layout mirrors for left-handed use;
 *  - the classic pads stay available as a selectable mode.
 *
 * NOT settled (user-owned design choices): every size/threshold in
 * TouchConfig is a placeholder default pending the user's design, and so
 * is multi-pointer steering ownership (currently: the most recent driving
 * pointer steers). Do not read these as final.
 */
#ifndef OPENCMR2_TOUCH_LAYOUT_H
#define OPENCMR2_TOUCH_LAYOUT_H

enum TouchMode {
    TOUCH_MODE_CLASSIC,
    TOUCH_MODE_FLOATING,
};

enum TouchHand {
    TOUCH_HAND_RIGHT,
    TOUCH_HAND_LEFT,
};

struct TouchConfig {
    TouchMode mode = TOUCH_MODE_FLOATING;
    TouchHand hand = TOUCH_HAND_RIGHT;

    // Placeholder tunables (unsettled design): fractions of the smaller
    // window dimension, or of the window dimension they name.
    float circleRadius = 0.12f;        // drawn circle, floating mode
    float steerFullDeflection = 0.10f; // |dx| for full steering lock
    float driveFullDeflection = 0.10f; // |dy| for full accelerate/brake
    float neutralRadius = 0.015f;      // dead zone around a fresh anchor
    float bothDeflection = 0.16f;      // brake circle: dy for brake+handbrake
    float handbrakeDeflection = 0.05f; // brake circle: -dy for handbrake
};

// Aggregated driving inputs from all live pointers.
struct TouchInput {
    float steer = 0.0f;      // -1 left .. +1 right
    float accelerate = 0.0f; // 0..1
    float brake = 0.0f;      // 0..1 (see the aggregation invariant above)
    bool handbrake = false;
};

// What a renderer draws for one active or anchored pad.
struct TouchPad {
    bool active = false;
    float anchorX = 0.0f, anchorY = 0.0f;
    float currentX = 0.0f, currentY = 0.0f;
    float radius = 0.0f;
};

class TouchControls {
public:
    explicit TouchControls(const TouchConfig &config = TouchConfig()) { m_config = config; }

    const TouchConfig &Config() const { return m_config; }
    void SetConfig(const TouchConfig &config); // re-anchors nothing; live pointers keep their anchors

    void PointerDown(int id, float x, float y);
    void PointerMove(int id, float x, float y);
    void PointerUp(int id);

    TouchInput Input() const;

    // Rendering support: the driving pad, the brake pad (floating mode) or
    // the fixed classic pads. Inactive pads return active == false.
    TouchPad DrivePad() const;
    TouchPad BrakePad() const;

    // Fixed classic-mode regions in normalized window coordinates (already
    // mirrored for the configured hand), each with its pressed state and
    // the pressing pointer's live position when held. Placeholder geometry
    // pending the settled design. Returns the region count (4) or 0 in
    // floating mode.
    struct TouchRegion {
        float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
        int role = 0; // matches the input roles: 1 steering pad, 3 accelerate, 4 brake, 5 handbrake
        bool pressed = false;
        float pressX = 0.0f, pressY = 0.0f;
    };
    int ClassicRegions(TouchRegion out[4]) const;

private:
    enum Role {
        ROLE_NONE,
        ROLE_DRIVE,          // floating driving circle; steering pad in classic mode
        ROLE_BRAKE_CIRCLE,   // floating brake/handbrake circle
        ROLE_BUTTON_ACCEL,   // classic fixed buttons
        ROLE_BUTTON_BRAKE,
        ROLE_BUTTON_HANDBRAKE,
    };
    enum BrakeState { BRAKE_NEUTRAL, BRAKE_BRAKING, BRAKE_BOTH, BRAKE_HANDBRAKE };

    struct Pointer {
        int id = -1;
        Role role = ROLE_NONE;
        int sequence = 0;             // touch-down order; steering ownership placeholder
        float anchorX = 0.0f, anchorY = 0.0f;
        float x = 0.0f, y = 0.0f;
        BrakeState brakeState = BRAKE_NEUTRAL;
    };

    Role RoleFor(float x, float y) const;
    void UpdateBrakeState(Pointer &p) const;
    const Pointer *Newest(Role role) const;

    TouchConfig m_config;
    Pointer m_pointers[8];
    int m_nextSequence = 1;
};

#endif // OPENCMR2_TOUCH_LAYOUT_H
