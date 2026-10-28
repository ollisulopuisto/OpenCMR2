#include "port/types.h"
#include "DustPlume.h"
#include <cstdio>
#include <vector>
#include <cmath>
int main(int argc,char **argv){
 if(argc<2)return 1;FILE *out=fopen(argv[1],"w");if(!out)return 1;
 std::vector<Particle> clouds;FixVector wind={0x600,0,0x300};
 fputs("frame,x,y,z,size,alpha,carx,carz\n",out);
 for(int frame=0;frame<270;++frame){
   float t=frame*.015f;
   float cx=std::sin(t)*15,cz=std::cos(t)*15;
   if(frame<180 && frame%3==0){Particle p={};p.age=DUST_PLUME_LIFE<<16;p.active=1;
     p.sourceVector={(int)(cx*65536),0,(int)(cz*65536)};
     p.position={(int)(std::cos(t)*0x900),0x800,(int)(-std::sin(t)*0x900)};
     clouds.push_back(p);}
   for(auto &p:clouds){if(!p.active)continue;p.age-=0x10000;if(p.age<=0){p.active=0;continue;}
     DustPlume_Step(p,wind);
     fprintf(out,"%d,%f,%f,%f,%f,%d,%f,%f\n",frame,p.sourceVector.x/65536.,p.sourceVector.y/65536.,p.sourceVector.z/65536.,p.size/65536.,p.type0x56,cx,cz);
   }
 }
 fclose(out);return 0;
}
