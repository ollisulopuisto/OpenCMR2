#include "TuningPanel.h"
#include <cstdio>
#include <cstring>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main(){
    int n; const TuningRow *r = Tuning_Rows(n);
    CHECK(n == 10);
    for (int i = 0; i < n; ++i) { CHECK(r[i].lo < r[i].hi); CHECK(r[i].step > 0); CHECK(r[i].fallback >= r[i].lo && r[i].fallback <= r[i].hi); CHECK(!std::strncmp(r[i].key, "video.", 6)); }
    CHECK(Tuning_Step(r[0], 0, -1) == 0);          // speed fov clamps at 0
    CHECK(Tuning_Step(r[0], 39, 1) == 40);         // and at 40
    CHECK(Tuning_Step(r[0], 12, 1) == 14);
    CHECK(Tuning_Step(r[1], 0, 1) == 1 && Tuning_Step(r[1], 1, 1) == 1);  // toggle rows stay 0/1
    CHECK(Tuning_Step(r[2], 300, 1) == 300);
    CHECK(Tuning_Step(r[6], 1, -1) == 1);          // dof max px never below 1
    CHECK(Tuning_Step(r[3], 500, 1) == 100);       // out of range snaps in
    CHECK(Tuning_Step(r[3], -50, -1) == 0);
    CHECK(Tuning_Select(0, -1, n) == n - 1 && Tuning_Select(n - 1, 1, n) == 0 && Tuning_Select(3, 1, n) == 4);
    if (!fails) std::puts("tuning_panel ok");
    return fails;
}
