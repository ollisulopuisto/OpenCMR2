#ifndef OPENCMR2_TEMPORAL_HISTORY_H
#define OPENCMR2_TEMPORAL_HISTORY_H

#include <cmath>
#include <cstdint>
#include <unordered_map>

// CPU foundation only. This does not enable temporal reconstruction or jitter
// the live camera. Callers must supply stable object IDs, not draw order or a
// vertex-buffer address (instanced objects can share buffers).
namespace Temporal {
struct Matrix { float m[16] = {}; }; // row-major, row-vector like GfxMatrix
struct Sample { float x = 0, y = 0; };
struct Config {
    int inputWidth = 0, inputHeight = 0, outputWidth = 0, outputHeight = 0;
    uint64_t scene = 0, camera = 0;
    bool operator==(const Config &b) const {
        return inputWidth == b.inputWidth && inputHeight == b.inputHeight &&
            outputWidth == b.outputWidth && outputHeight == b.outputHeight &&
            scene == b.scene && camera == b.camera;
    }
    bool valid() const {
        return inputWidth > 0 && inputHeight > 0 && outputWidth >= inputWidth && outputHeight >= inputHeight;
    }
};
inline float RadicalInverse(unsigned n, unsigned base) {
    float result = 0, weight = 1.0f;
    while (n) { weight /= base; result += (n % base) * weight; n /= base; }
    return result;
}
inline Sample Jitter(unsigned index) {
    unsigned n = index % 8 + 1;
    return {RadicalInverse(n, 2) - 0.5f, RadicalInverse(n, 3) - 0.5f};
}
// MetalFX motion convention: current pixel -> previous pixel in input pixels,
// top-left origin. Matrices must be UNJITTERED. Reject points behind camera.
inline bool Motion(const Matrix &current, const Matrix &previous, float x, float y, float z,
                   int width, int height, Sample &result) {
    if (width <= 0 || height <= 0) return false;
    auto project = [=](const Matrix &a, Sample &p) {
        float w = x*a.m[3] + y*a.m[7] + z*a.m[11] + a.m[15];
        if (!std::isfinite(w) || w <= 1e-6f) return false;
        p = {(x*a.m[0]+y*a.m[4]+z*a.m[8]+a.m[12])/w,
             (x*a.m[1]+y*a.m[5]+z*a.m[9]+a.m[13])/w};
        return std::isfinite(p.x) && std::isfinite(p.y);
    };
    Sample c, p;
    if (!project(current, c) || !project(previous, p)) return false;
    result = {(p.x-c.x)*width*0.5f, (c.y-p.y)*height*0.5f};
    return true;
}
class History {
    Config config;
    uint64_t serial = 0;
    unsigned sequence = 0;
    bool valid = false, recording = false;
    std::unordered_map<uint64_t, Matrix> previous, pending;
public:
    // Call at camera cuts, stage changes, seeks, pause/resume and device loss.
    // Resize and non-consecutive frame numbers also invalidate automatically.
    void Reset() { previous.clear(); pending.clear(); valid = recording = false; sequence = 0; }
    bool Begin(const Config &next, uint64_t frame) {
        if (recording || !next.valid()) { Reset(); return false; }
        if (!valid || !(config == next) || frame != serial + 1) Reset();
        config = next; serial = frame; recording = true; pending.clear();
        return true;
    }
    Sample CurrentJitter() const { return Jitter(sequence); }
    bool HasHistory() const { return valid; }
    // Duplicate IDs are ambiguous. Fail closed for the entire temporal frame.
    bool Record(uint64_t id, const Matrix &current, Matrix &prior) {
        if (!recording) return false;
        for (float v : current.m) if (!std::isfinite(v)) { Reset(); return false; }
        if (!pending.emplace(id, current).second) { Reset(); return false; }
        auto old = previous.find(id);
        if (!valid || old == previous.end()) { prior = current; return false; }
        prior = old->second; return true;
    }
    // Commit only after a successful reconstruction/submission. Failed/skipped
    // frames must never become a valid history source. Objects absent this
    // frame are discarded so reappearing objects do not inherit stale motion.
    void End(bool submitted) {
        if (!recording || !submitted) { Reset(); return; }
        previous.swap(pending); pending.clear(); valid = true; recording = false;
        sequence = (sequence + 1) % 8;
    }
};
}
#endif
