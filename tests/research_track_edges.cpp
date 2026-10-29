// Local research regression. No default/production caller enables exact edges.
#include "port/types.h"
#include "FixedPoint.h"
#include <cstdio>
#include <climits>
extern int Track_PointInTriangle(FixVector *,short,FixVector *);
extern void Track_ResearchExactEdges(bool);
#define CHECK(x) do{if(!(x)){fprintf(stderr,"exact edge line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 // Source-backed fixed coordinates of two shared curved research triangles.
 FixVector a[]={{119884392,2975357,83378672},{119549969,2991121,83337544},{119224552,3008857,83388512}};
 FixVector b[]={{119549969,2991121,83337544},{119232064,3006885,83293832},{119224552,3008857,83388512}};
 FixVector point={119447002,2997032,83353671};
 Track_ResearchExactEdges(false);CHECK(!Track_PointInTriangle(&point,0,a));
 // Exact sign on triangle a says outside. Opposite shared side must own it.
 Track_ResearchExactEdges(true);CHECK(!Track_PointInTriangle(&point,0,a));
 CHECK(Track_PointInTriangle(&point,0,b));
 FixVector degenerate[3]={{0,0,0},{1,0,1},{2,0,2}},p={1,0,1};CHECK(!Track_PointInTriangle(&p,0,degenerate));
 FixVector huge[3]={{INT_MIN,0,0},{INT_MAX,0,0},{0,0,INT_MAX}};CHECK(!Track_PointInTriangle(&p,0,huge));
 Track_ResearchExactEdges(false);puts("research exact edges: shared fixed-point seam, degenerate and overflow guards pass");
}
