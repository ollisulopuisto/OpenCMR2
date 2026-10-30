// OpenCMR2: optional bounded history of tyre-mark points (video.tire_track_history).
// A circular store with a hard capacity, split into chunks of kTrailChunk points. Each chunk keeps an x/z bounding box
// so that drawing can skip whole chunks far from a reference point. Oldest points are dropped when the store is full,
// so this is "long", not unlimited: marks can expire once the capacity is exceeded.
#pragma once
#include <cstddef>
#include <vector>
using std::size_t;

constexpr int kTrailChunk = 64;

template <class P>
struct TrailHistory {
    std::vector<P> pts;
    std::vector<int> minX, maxX, minZ, maxZ;
    int cap = 0;
    int head = 0;
    int count = 0;

    bool Active() const { return cap > 0; }
    void Clear() { std::vector<P>().swap(pts); minX.clear(); maxX.clear(); minZ.clear(); maxZ.clear(); cap = head = count = 0; }
    // capacity is rounded up to a multiple of kTrailChunk, at least two chunks.
    void Init(int capacity)
    {
        int c = (capacity + kTrailChunk - 1) / kTrailChunk * kTrailChunk;
        if (c < 2 * kTrailChunk) c = 2 * kTrailChunk;
        if (c == cap) return;
        Clear();
        cap = c;
        pts.resize((size_t)cap);
        int n = cap / kTrailChunk;
        minX.assign((size_t)n, 0); maxX.assign((size_t)n, 0); minZ.assign((size_t)n, 0); maxZ.assign((size_t)n, 0);
    }
    int Phys(int n) const { return (head - count + n + cap * 2) % cap; }
    const P &At(int n) const { return pts[(size_t)Phys(n)]; }
    void Push(const P &p, int x, int z)
    {
        if (cap == 0) return;
        const int ch = head / kTrailChunk;
        if (head % kTrailChunk == 0) {
            // starting to overwrite this chunk: its older points are the oldest in the store, drop them all
            if (count > cap - kTrailChunk) count = cap - kTrailChunk;
            minX[(size_t)ch] = maxX[(size_t)ch] = x;
            minZ[(size_t)ch] = maxZ[(size_t)ch] = z;
        } else {
            if (x < minX[(size_t)ch]) minX[(size_t)ch] = x;
            if (x > maxX[(size_t)ch]) maxX[(size_t)ch] = x;
            if (z < minZ[(size_t)ch]) minZ[(size_t)ch] = z;
            if (z > maxZ[(size_t)ch]) maxZ[(size_t)ch] = z;
        }
        pts[(size_t)head] = p;
        head = (head + 1) % cap;
        if (count < cap) ++count;
    }
    bool ChunkFar(int ch, int rx, int rz, long long r) const
    {
        long long dx = 0, dz = 0;
        if (rx < minX[(size_t)ch]) dx = (long long)minX[(size_t)ch] - rx; else if (rx > maxX[(size_t)ch]) dx = (long long)rx - maxX[(size_t)ch];
        if (rz < minZ[(size_t)ch]) dz = (long long)minZ[(size_t)ch] - rz; else if (rz > maxZ[(size_t)ch]) dz = (long long)rz - maxZ[(size_t)ch];
        return dx * dx + dz * dz > r * r;
    }
    // Calls f(cur, prev) newest to oldest for consecutive stored points. f returns true when it queued geometry; at most
    // maxSegments segments are queued. Chunks entirely farther than radius from (rx, rz) are skipped, which is the only
    // place a segment can be missed besides the budget. Work is bounded by cap / kTrailChunk chunk tests plus the points
    // of near chunks. Returns the number of segments queued.
    template <class F>
    int Walk(int rx, int rz, long long radius, int maxSegments, F f) const
    {
        int drawn = 0;
        int n = count - 1;
        while (n >= 1 && drawn < maxSegments) {
            const int p = Phys(n);
            const int ch = p / kTrailChunk;
            if (ChunkFar(ch, rx, rz, radius)) {
                n -= (p % kTrailChunk) + 1;
                continue;
            }
            if (f(At(n), At(n - 1))) ++drawn;
            --n;
        }
        return drawn;
    }
};
