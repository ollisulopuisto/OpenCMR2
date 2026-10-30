#include "ReplayCameraCycle.h"
#include <cstdio>
#include <initializer_list>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static unsigned mask(std::initializer_list<int> l) { unsigned m = 0; for (int i : l) m |= 1u << i; return m; }
// The batch-7 rule: target = mode+1 (skipping only a not-allowed 1..5), past 5 go straight to 7, whether or not 7 can be selected.
static int OldNext(int mode, unsigned available)
{
    if (mode == 7) return 4;
    int next = mode;
    do { next++; if (next > 5) return 7; } while (!(available & (1u << next)));
    return next;
}
int main()
{
    CHECK(OldNext(5, mask({1, 2, 3, 4, 5, 7})) == 7);       // red 1: batch-7 went 5 -> automatic, so car, bonnet and cockpit were never reachable
    CHECK(OldNext(5, mask({1, 2, 3, 4, 5})) == 7);          // red 2: and it chose 7 even when View_SwitchCamera refuses it (no camera spots): the press did nothing
    unsigned all = mask({1, 2, 3, 4, 5, 7});
    int order[7] = {7, 4, 5, 1, 2, 3, 7}, m = 7;
    for (int i = 1; i < 7; i++) { m = ReplayCamera_Next(m, all); CHECK(m == order[i]); }   // full lap, one step per press, wraps to automatic
    // No camera spots (7 refused by View_SwitchCamera): the wrap must skip it, not stall.
    unsigned no7 = mask({1, 2, 3, 4, 5});
    CHECK(ReplayCamera_Next(3, no7) == 4);
    CHECK(ReplayCamera_Next(4, no7) == 5);
    // Cockpit not loaded: skipped, wraps to automatic.
    unsigned no3 = mask({1, 2, 4, 5, 7});
    CHECK(ReplayCamera_Next(2, no3) == 7);
    // The old code stalled from modes outside the list; they start from the top now.
    for (int cur : {0, 6, 8, 9, 10}) CHECK(ReplayCamera_Next(cur, all) == 7);
    CHECK(ReplayCamera_Next(6, no7) == 4);
    // Only the current mode available, or none: no switch.
    CHECK(ReplayCamera_Next(4, mask({4})) == -1);
    CHECK(ReplayCamera_Next(4, 0) == -1);
    puts("replay camera cycle ok");
    return 0;
}
