#include "TouchLayout.h"

#include <cstddef>

namespace {

float Clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }
float ClampSym(float v, float limit) { return v < -limit ? -limit : v > limit ? limit : v; }

bool InRect(float x, float y, float x0, float y0, float x1, float y1)
{
    return x >= x0 && x < x1 && y >= y0 && y < y1;
}

} // namespace

void TouchControls::SetConfig(const TouchConfig &config)
{
    m_config = config;
}

TouchControls::Role TouchControls::RoleFor(float x, float y) const
{
    if (m_config.mode == TOUCH_MODE_FLOATING) {
        // The screen halves pick the circle, mirrored for left-handed use.
        bool leftHalf = x < 0.5f;
        if (m_config.hand == TOUCH_HAND_LEFT)
            leftHalf = !leftHalf;
        return leftHalf ? ROLE_DRIVE : ROLE_BRAKE_CIRCLE;
    }
    // Classic fixed pads (placeholder regions pending the settled design):
    // steering pad on the driving side's lower half, accelerate/brake/
    // handbrake buttons on the other side, mirrored by handedness.
    TouchRegion regions[4];
    ClassicRegions(regions);
    for (const TouchRegion &r : regions)
        if (InRect(x, y, r.x0, r.y0, r.x1, r.y1))
            return (Role)r.role;
    return ROLE_NONE;
}

// Right-handed fixed layout; left-handed mirrors x. The ordering matches
// Role: steering pad, accelerate, brake, handbrake.
static const TouchControls::TouchRegion kClassicRight[4] = {
    {0.0f, 0.5f, 0.5f, 1.0f, 1, false, 0.0f, 0.0f},
    {0.5f, 0.08f, 1.0f, 0.38f, 3, false, 0.0f, 0.0f},
    {0.66f, 0.42f, 1.0f, 0.72f, 4, false, 0.0f, 0.0f},
    {0.5f, 0.42f, 0.66f, 0.72f, 5, false, 0.0f, 0.0f},
};

int TouchControls::ClassicRegions(TouchRegion out[4]) const
{
    if (m_config.mode != TOUCH_MODE_CLASSIC || out == NULL)
        return 0;
    bool mirror = m_config.hand == TOUCH_HAND_LEFT;
    for (int i = 0; i < 4; i++) {
        TouchRegion r = kClassicRight[i];
        if (mirror) {
            float x0 = r.x0;
            r.x0 = 1.0f - r.x1;
            r.x1 = 1.0f - x0;
        }
        const Pointer *p = Newest((Role)r.role);
        if (p != NULL) {
            r.pressed = true;
            r.pressX = p->x;
            r.pressY = p->y;
        }
        out[i] = r;
    }
    return 4;
}

void TouchControls::PointerDown(int id, float x, float y)
{
    Pointer *slot = NULL;
    for (Pointer &p : m_pointers) {
        if (p.id == id)
            return; // a platform repeating a down for a live pointer: ignore
        if (slot == NULL && p.id == -1)
            slot = &p;
    }
    if (slot == NULL)
        return;
    slot->id = id;
    slot->sequence = m_nextSequence++;
    slot->anchorX = x;
    slot->anchorY = y;
    slot->x = x;
    slot->y = y;
    slot->brakeState = BRAKE_NEUTRAL;
    slot->role = RoleFor(x, y);
}

void TouchControls::PointerMove(int id, float x, float y)
{
    for (Pointer &p : m_pointers) {
        if (p.id != id)
            continue;
        p.x = x;
        p.y = y;
        if (p.role == ROLE_BRAKE_CIRCLE)
            UpdateBrakeState(p);
        return;
    }
}

void TouchControls::PointerUp(int id)
{
    for (Pointer &p : m_pointers) {
        if (p.id != id)
            continue;
        p.id = -1;
        p.role = ROLE_NONE;
        p.brakeState = BRAKE_NEUTRAL;
        return;
    }
}

void TouchControls::UpdateBrakeState(Pointer &p) const
{
    // Relative, direction-based states (y down): pull down to brake,
    // farther for both, push up for the handbrake. Once activated the
    // state is sticky through the center, so reversing out of braking
    // becomes the handbrake without a neutral stop; the small neutral
    // radius only debounces a fresh touch-down. A single move event can
    // cross more than one threshold, so the states settle in a loop.
    float dy = p.y - p.anchorY;
    for (int steps = 0; steps < 3; steps++) {
        BrakeState before = p.brakeState;
        switch (p.brakeState) {
    case BRAKE_NEUTRAL:
        if (dy > m_config.neutralRadius)
            p.brakeState = BRAKE_BRAKING;
        else if (dy < -m_config.neutralRadius)
            p.brakeState = BRAKE_HANDBRAKE;
        break;
    case BRAKE_BRAKING:
        if (dy > m_config.bothDeflection)
            p.brakeState = BRAKE_BOTH;
        else if (dy < -m_config.handbrakeDeflection)
            p.brakeState = BRAKE_HANDBRAKE;
        break;
    case BRAKE_BOTH:
        if (dy < -m_config.handbrakeDeflection)
            p.brakeState = BRAKE_HANDBRAKE;
        else if (dy < m_config.bothDeflection - m_config.neutralRadius)
            p.brakeState = BRAKE_BRAKING;
        break;
    case BRAKE_HANDBRAKE:
        if (dy > m_config.neutralRadius)
            p.brakeState = BRAKE_BRAKING;
        break;
        }
        if (p.brakeState == before)
            break;
    }
}

const TouchControls::Pointer *TouchControls::Newest(Role role) const
{
    const Pointer *found = NULL;
    for (const Pointer &p : m_pointers)
        if (p.id != -1 && p.role == role && (found == NULL || p.sequence > found->sequence))
            found = &p;
    return found;
}

TouchInput TouchControls::Input() const
{
    TouchInput out;
    // Steering ownership: the most recent driving pointer (placeholder,
    // unsettled design). Accelerate/brake/handbrake aggregate across every
    // pointer: a held brake keeps braking while ANY pointer holds it, so
    // releasing one brake source never cancels another (settled invariant).
    const Pointer *driver = Newest(ROLE_DRIVE);
    if (driver != NULL) {
        float dx = driver->x - driver->anchorX;
        out.steer = ClampSym(dx / m_config.steerFullDeflection, 1.0f);
        if (m_config.mode == TOUCH_MODE_FLOATING) {
            float dy = driver->y - driver->anchorY;
            if (dy < 0.0f)
                out.accelerate = Clamp01(-dy / m_config.driveFullDeflection);
            else
                out.brake = Clamp01(dy / m_config.driveFullDeflection);
        }
    }
    for (const Pointer &p : m_pointers) {
        if (p.id == -1)
            continue;
        switch (p.role) {
        case ROLE_BRAKE_CIRCLE: {
            float dy = p.y - p.anchorY;
            if (p.brakeState == BRAKE_BRAKING || p.brakeState == BRAKE_BOTH) {
                float amount = Clamp01(dy / m_config.bothDeflection);
                if (amount > out.brake)
                    out.brake = amount;
            }
            if (p.brakeState == BRAKE_BOTH || p.brakeState == BRAKE_HANDBRAKE)
                out.handbrake = true;
            break;
        }
        case ROLE_BUTTON_ACCEL:
            out.accelerate = 1.0f;
            break;
        case ROLE_BUTTON_BRAKE:
            out.brake = 1.0f;
            break;
        case ROLE_BUTTON_HANDBRAKE:
            out.handbrake = true;
            break;
        default:
            break;
        }
    }
    return out;
}

TouchPad TouchControls::DrivePad() const
{
    TouchPad pad;
    const Pointer *p = Newest(ROLE_DRIVE);
    if (p != NULL) {
        pad.active = true;
        pad.anchorX = p->anchorX;
        pad.anchorY = p->anchorY;
        pad.currentX = p->x;
        pad.currentY = p->y;
        pad.radius = m_config.circleRadius;
    }
    return pad;
}

TouchPad TouchControls::BrakePad() const
{
    TouchPad pad;
    const Pointer *p = Newest(ROLE_BRAKE_CIRCLE);
    if (p != NULL) {
        pad.active = true;
        pad.anchorX = p->anchorX;
        pad.anchorY = p->anchorY;
        pad.currentX = p->x;
        pad.currentY = p->y;
        pad.radius = m_config.circleRadius;
    }
    return pad;
}
