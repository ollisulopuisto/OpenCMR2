#include "TuningMode.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main()
{
    int seen[4] = {0, 0, 0, 0}, mode = 0;
    for (int i = 0; i < 8; i++) { seen[mode]++; mode = TuningMode_Next(mode); }
    CHECK(mode == 0);                                         // the cycle closes after four presses
    for (int i = 0; i < 4; i++) CHECK(seen[i] == 2);          // and visits every state exactly once per lap
    CHECK(TuningMode_FullInfo(0) && TuningMode_FullInfo(1) && !TuningMode_FullInfo(2) && !TuningMode_FullInfo(3));
    CHECK(TuningMode_AdjustShown(1) && !TuningMode_AdjustShown(0) && !TuningMode_AdjustShown(2) && !TuningMode_AdjustShown(3)); // adjustment keys stay captured only in mode 1
    CHECK(!TuningMode_InfoShown(2) && TuningMode_InfoShown(3));
    CHECK(TuningMode_FpsOnly(3) && !TuningMode_FpsOnly(0) && !TuningMode_FpsOnly(1) && !TuningMode_FpsOnly(2));
    puts("tuning mode ok");
    return 0;
}
