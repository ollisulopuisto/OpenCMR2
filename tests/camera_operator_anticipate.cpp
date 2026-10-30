#include "CameraOperator.h"
#include <cstdio>
#include <vector>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
using namespace CameraOperator;
int main(){
 // A straight route that turns at node 5. Lookahead 1 keeps the old 12/30 sample distances; 2 doubles them.
 std::vector<V3> n;for(int i=0;i<12;i++)n.push_back(i<5?V3{0,0,i*10.f}:V3{(i-4)*10.f,0,40.f});
 auto pt=[&](int i){return n[i];};
 auto a=PreviewRoad({0,0,0},1,12,false,pt),b=PreviewRoad({0,0,0},1,12,false,pt,2.f);
 CHECK(a.valid&&b.valid);CHECK(std::fabs(a.near.z-12.f)<.01f);CHECK(std::fabs(b.near.z-24.f)<.01f);
 CHECK(std::fabs(b.bend)>std::fabs(a.bend)||std::fabs(b.bend)>.5f); // the turn is seen earlier when looking further
 auto c=PreviewRoad({0,0,0},1,12,false,pt,9.f);CHECK(std::fabs(c.near.z-24.f)<.01f); // clamped to 2
 // Low-pass: a step in the raw bend must reach the framing gradually and the filter is relative to the car (a fast car does not drag it behind).
 State s;RoadPreview r;r.valid=true;r.tangent={0,0,1};r.near={0,0,12};r.far={0,0,30};
 V3 car={0,0,0};for(int i=0;i<50;i++){car.z+=1.f;r.near=car+V3{0,0,12};r.far=car+V3{0,0,30};s.Step(car,0,0,1.f,6.5,2,.04,&r);}
 CHECK(std::fabs(s.nearSmooth.z-12.f)<.01f);CHECK(std::fabs(s.bendSmooth)<.001f);
 r.bend=1.f;r.far=car+V3{20,0,20};s.Step(car,0,0,1.f,6.5,2,.04,&r);
 CHECK(s.bendSmooth>.05f&&s.bendSmooth<.2f); // one 0.04 s step moves about 12% of the way, not all of it
 for(int i=0;i<100;i++)s.Step(car,0,0,1.f,6.5,2,.04,&r);CHECK(s.bendSmooth>.95f);
 // Losing the preview clears the filter so a later preview starts fresh.
 s.Step(car,0,0,1.f,6.5,2,.04,nullptr);CHECK(!s.previewReady);
 puts("camera operator anticipate ok");return 0;}
