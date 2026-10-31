#include "SceneryMip.h"
#include <cstdio>
#define CHECK(x) do{if(!(x))return __LINE__;}while(0)
int main(){
 std::vector<uint8_t> p={0,255,0,0, 0,0,255,255, 255,0,0,0, 255,255,0,0};auto m=SceneryMip::Down(p,2,2,.25);
 CHECK(m.size()==4);CHECK(m[0]==0&&m[1]==0&&m[2]==255); // hidden RGB never bleeds
 std::vector<uint8_t> solid(3*5*4);for(size_t i=0;i<solid.size();i+=4){solid[i]=17;solid[i+1]=33;solid[i+2]=77;solid[i+3]=255;}
 auto odd=SceneryMip::Down(solid,3,5,1);CHECK(odd.size()==8);for(int i=0;i<2;i++)CHECK(odd[i*4]==17&&odd[i*4+3]==255);
 std::vector<uint8_t> empty(8*4,0);auto e=SceneryMip::Down(empty,8,1,0);for(auto b:e)CHECK(!b);
 // Coverage correction chooses a reachable count rather than erasing a cutout.
 std::vector<uint8_t> thin(4*4*4,0);for(int i=0;i<4;i++){thin[(i*4+i)*4+3]=255;thin[(i*4+i)*4]=90;}
 auto t=SceneryMip::Down(thin,4,4,.25);unsigned count=0;for(size_t i=3;i<t.size();i+=4)count+=t[i]>=128;CHECK(count==2); // two equal-alpha bins, nearest attainable to1
 auto q=SceneryMip::Down(p,2,2,.25);CHECK(q==m);puts("tree premultiplied mip hidden RGB/odd/opaque/empty/determinism pass");
}
