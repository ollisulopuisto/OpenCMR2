// iPad menu gestures. Pure logic; coordinates are normalized window space.
#ifndef OPENCMR2_TOUCH_NAVIGATION_H
#define OPENCMR2_TOUCH_NAVIGATION_H
#include <cmath>
#include <cstdint>
#include <map>

class TouchNavigation {
public:
    enum Action { None, Left, Right, Up, Down, Confirm, Back };
    // A context/page transition cancels the entire current gesture. Keep IDs
    // until they lift so an old finger cannot start a new screen's action.
    void Cancel() { cancelled = !fingers.empty(); }
    void Reset() { fingers.clear(); cancelled = false; }
    Action Event(int type, long long id, float x, float y, uint64_t now, float width, float height, bool outside = false)
    {
        if (type == 0) {
            if (fingers.empty()) {
                cancelled = false;
                moved = false;
                peak = 0;
                direction = None;
                started = now;
            }
            fingers.emplace(id, Finger{x, y});
            if (outside) cancelled = true;
            if ((int)fingers.size() > peak) peak = (int)fingers.size();
            if (peak > 2) cancelled = true;
            return None;
        }
        if (outside) cancelled = true;
        auto found = fingers.find(id);
        if (found == fingers.end()) return None;
        // Physical distance relative to the short edge, not different
        // normalized thresholds on each axis of a landscape iPad.
        float basis = width < height ? width : height;
        if (basis <= 0) { cancelled = true; basis = 1; }
        float dx = (x - found->second.x) * width / basis;
        float dy = (y - found->second.y) * height / basis;
        float distance = std::sqrt(dx * dx + dy * dy);
        if (distance > 0.025f) moved = true;
        if (direction == None && distance >= 0.08f) {
            // Lock the first qualifying dominant axis. Opposite finger
            // direction in all axes, as chosen by Olli for this sketch.
            direction = std::fabs(dx) >= std::fabs(dy)
                ? (dx < 0 ? Right : Left) : (dy < 0 ? Down : Up);
        }
        if (type != 2) return None;
        fingers.erase(found);
        if (!fingers.empty()) return None;
        if (cancelled || now < started || now - started > 700) return None;
        if (peak == 2) return moved ? None : Back;
        if (direction != None) return direction;
        return moved ? None : Confirm;
    }
private:
    struct Finger { float x, y; };
    std::map<long long, Finger> fingers;
    bool cancelled = false, moved = false;
    int peak = 0;
    Action direction = None;
    uint64_t started = 0;
};
#endif
