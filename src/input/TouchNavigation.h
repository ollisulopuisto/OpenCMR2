// iPad menu gestures. Pure logic; coordinates are normalized window space.
#ifndef OPENCMR2_TOUCH_NAVIGATION_H
#define OPENCMR2_TOUCH_NAVIGATION_H
#include <cmath>
#include <cstdint>
#include <map>

struct TouchPicture {
    int width = 0, height = 0;
    bool stretch = false;
    bool Outside(float x, float y, int windowW, int windowH) const {
        if (stretch || width <= 0 || height <= 0 || windowW <= 0 || windowH <= 0) return false;
        float scaleX = (float)windowW / width, scaleY = (float)windowH / height;
        float scale = scaleX < scaleY ? scaleX : scaleY;
        float marginX = (1.0f - width * scale / windowW) * 0.5f;
        float marginY = (1.0f - height * scale / windowH) * 0.5f;
        return x < marginX || x > 1.0f - marginX || y < marginY || y > 1.0f - marginY;
    }
};

struct TouchPictureRect {
    int x = 0, y = 0, width = 0, height = 0;
    bool Outside(const TouchPicture &picture, float px, float py, int windowW, int windowH) const {
        if (width <= 0 || height <= 0 || picture.width <= 0 || picture.height <= 0 || windowW <= 0 || windowH <= 0)
            return true; // no presented movie yet
        float sx = (float)windowW / picture.width, sy = (float)windowH / picture.height;
        if (!picture.stretch) sx = sy = sx < sy ? sx : sy;
        float offsetX = (windowW - picture.width * sx) * .5f;
        float offsetY = (windowH - picture.height * sy) * .5f;
        float bx = (px * windowW - offsetX) / sx, by = (py * windowH - offsetY) / sy;
        return bx < x || bx > x + width || by < y || by > y + height;
    }
};

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
