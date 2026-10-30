#pragma once
// Replay free camera. Visual only: it replaces the shown camera pose, never touches the car, the recorded controls or the physics.
// Pose = position + yaw/pitch. Axes are rebuilt from them each update, so the camera never rolls. Everything is in double precision and
// is converted to the game's 16.16 matrix by the caller. `hand` records whether the engine's right axis is +cross(up,forward) or the
// opposite, measured from the camera it starts from, so turning right always turns toward the screen's right.
#include <cmath>
struct FreeCamInput { bool forward, back, left, right, up, down, turnLeft, turnRight, lookUp, lookDown, fast; };
struct FreeCamAxes { double f[3], r[3], u[3]; };
struct FreeCam {
    bool active = false;
    double pos[3] = {0, 0, 0}, yaw = 0, pitch = 0;
    int hand = 1;
    static constexpr double kMove = 12.0, kFastMul = 4.0, kTurn = 1.4, kMaxPitch = 1.5;   // units/s, x, rad/s, rad (about 86 degrees)
    void Start(const double position[3], const double f[3], const double r[3])
    {
        for (int i = 0; i < 3; i++) pos[i] = position[i];
        double fy = f[1] < -1 ? -1 : (f[1] > 1 ? 1 : f[1]);
        pitch = std::asin(fy);
        yaw = std::atan2(f[0], f[2]);
        if (pitch > kMaxPitch) pitch = kMaxPitch;
        if (pitch < -kMaxPitch) pitch = -kMaxPitch;
        double dot = r[0] * std::cos(yaw) + r[2] * -std::sin(yaw);   // against cross(up, forward) on the ground plane
        hand = dot < 0 ? -1 : 1;
        active = true;
    }
    FreeCamAxes Axes() const
    {
        double sy = std::sin(yaw), cy = std::cos(yaw), sp = std::sin(pitch), cp = std::cos(pitch);
        FreeCamAxes a;
        a.f[0] = cp * sy; a.f[1] = sp; a.f[2] = cp * cy;
        a.r[0] = hand * cy; a.r[1] = 0; a.r[2] = hand * -sy;
        a.u[0] = -sp * sy; a.u[1] = cp; a.u[2] = -sp * cy;
        return a;
    }
    // dt in seconds (clamped to 0.1 so a stall cannot throw the camera). Works with zero sim steps: it uses real time only.
    void Update(const FreeCamInput &in, double dt)
    {
        if (!active) return;
        if (!(dt > 0)) return;
        if (dt > 0.1) dt = 0.1;
        yaw += ((in.turnRight ? 1 : 0) - (in.turnLeft ? 1 : 0)) * hand * kTurn * dt;
        pitch += ((in.lookUp ? 1 : 0) - (in.lookDown ? 1 : 0)) * kTurn * dt;
        if (pitch > kMaxPitch) pitch = kMaxPitch;
        if (pitch < -kMaxPitch) pitch = -kMaxPitch;
        const double twoPi = 6.283185307179586;
        if (yaw > twoPi) yaw -= twoPi; else if (yaw < -twoPi) yaw += twoPi;
        FreeCamAxes a = Axes();
        double speed = kMove * (in.fast ? kFastMul : 1.0) * dt;
        double mf = ((in.forward ? 1 : 0) - (in.back ? 1 : 0)) * speed;
        double mr = ((in.right ? 1 : 0) - (in.left ? 1 : 0)) * speed;
        double mu = ((in.up ? 1 : 0) - (in.down ? 1 : 0)) * speed;   // straight along world up, not along the view
        for (int i = 0; i < 3; i++) pos[i] += a.f[i] * mf + a.r[i] * mr;
        pos[1] += mu;
    }
};
