#include "TuningPanel.h"
#include <cstdio>
#include <cstring>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main(){
    int n; const TuningRow *r = Tuning_Rows(n);
    CHECK(n == 13);
    for (int i = 0; i < n; ++i) { CHECK(r[i].lo < r[i].hi); CHECK(r[i].step > 0); CHECK(r[i].fallback >= r[i].lo && r[i].fallback <= r[i].hi); CHECK(!std::strncmp(r[i].key, "video.", 6)); }
    CHECK(Tuning_Step(r[0], 0, -1) == 0);          // speed fov clamps at 0
    CHECK(Tuning_Step(r[0], 39, 1) == 40);         // and at 40
    CHECK(Tuning_Step(r[0], 12, 1) == 14);
    CHECK(Tuning_Step(r[1], 0, 1) == 1 && Tuning_Step(r[1], 1, 1) == 1);  // toggle rows stay 0/1
    CHECK(Tuning_Step(r[2], 300, 1) == 300);
    CHECK(Tuning_Step(r[7], 1, -1) == 1);          // dof max px never below 1
    CHECK(Tuning_Step(r[4], 500, 1) == 100);       // out of range snaps in
    CHECK(Tuning_Step(r[4], -50, -1) == 0);
    CHECK(Tuning_Step(r[3], 100, -1) == 50 && Tuning_Step(r[3], 800, 1) == 800 && Tuning_Step(r[3], 10, -1) == 50);  // mote size 50..800
    CHECK(Tuning_Select(0, -1, n) == n - 1 && Tuning_Select(n - 1, 1, n) == 0 && Tuning_Select(3, 1, n) == 4);
    // Key filtering: panel keys vanish only while open; the toggle key always; everything else untouched.
    unsigned char k[256]; for (int i = 0; i < 256; ++i) k[i] = 0x80;
    Tuning_FilterKeys(k, false, kTuningToggleDefault);
    CHECK(k[0x44] == 0); for (int i : kTuningKeys) CHECK(k[i] == 0x80);
    CHECK(k[0x1b] == 0x80 && k[0x1a] == 0x80 && k[0x39] == 0x80 && k[0xc8] == 0x80);   // gear keys, space, arrows stay
    Tuning_FilterKeys(k, true, kTuningToggleDefault);
    for (int i : kTuningKeys) CHECK(k[i] == 0);
    CHECK(k[0x1b] == 0x80 && k[0x2e] == 0x80);
    { unsigned char z[256] = {}; Tuning_FilterKeys(z, true, 0); Tuning_FilterKeys(z, true, 999); CHECK(z[0x44] == 0); }
    // No tuning key is a default driving key (arrows, space, [ ], C, numpad).
    static const int driving[] = {0xcb, 0xcd, 0xc8, 0xd0, 0x39, 0x1b, 0x1a, 0x2e, 0x4b, 0x4d};
    for (int d : driving) { for (int t : kTuningKeys) CHECK(d != t); CHECK(d != kTuningToggleDefault); }
    // Toggle key validation: default F10 is fine; driving/menu keys, digits and out-of-range values are refused.
    CHECK(Tuning_ToggleKeyAllowed(kTuningToggleDefault) && Tuning_ToggleKeyAllowed(0x43) && Tuning_ToggleKeyAllowed(0x0F));
    for (int d : driving) CHECK(!Tuning_ToggleKeyAllowed(d));
    for (int t : kTuningKeys) CHECK(!Tuning_ToggleKeyAllowed(t));
    CHECK(!Tuning_ToggleKeyAllowed(0) && !Tuning_ToggleKeyAllowed(256) && !Tuning_ToggleKeyAllowed(-3) && !Tuning_ToggleKeyAllowed(0x1c) && !Tuning_ToggleKeyAllowed(0x01));
    // Edge detection: one trigger per press, held key does not repeat, release re-arms.
    TuningEdges e; int fires = 0;
    bool seq[] = {0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1};
    for (bool d : seq) fires += e.Edge(2, d);
    CHECK(fires == 3);
    CHECK(!e.Edge(3, false));
    if (!fails) std::puts("tuning_panel ok");
    return fails;
}
