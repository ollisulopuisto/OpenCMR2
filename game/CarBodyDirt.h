#ifndef OPENCMR2_CAR_BODY_DIRT_H
#define OPENCMR2_CAR_BODY_DIRT_H
#pragma push_macro("rand")
#undef rand
#include <algorithm>
#pragma pop_macro("rand")
#include <cmath>
#include <cstring>
#include <cctype>
// Cosmetic state only: no saved-game or original mesh/texture byte changes.
namespace CarBodyDirt {
struct Layer {float amount=0,red=.42f,green=.36f,blue=.26f;};
inline Layer cars[8]={};
inline int drawingCar=-1;
// Evidence-backed original/mod body-atlas names. Unknown/custom names
// deliberately stay clean, not inferred from hierarchy or broad substrings.
inline bool IsPaintAtlas(const char *path){
 if(!path)return false;
 const char *name=path;for(const char*p=path;*p;p++)if(*p=='/'||*p=='\\')name=p+1;
 char stem[32]={};size_t n=0;
 while(name[n]&&name[n]!='.'&&n<sizeof(stem)-1){stem[n]=char(std::tolower((unsigned char)name[n]));n++;}
 static const char *known[]={"affdbodf","amldbodf","ap2dbodf","ap5dbodf","ascdbodf","asidbodf"};
 for(auto k:known)if(std::strcmp(stem,k)==0)return true;
 return false;
}
inline void Reset(){for(auto &c:cars)c={};drawingCar=-1;}
inline void Deposit(int car,float exposure,float red,float green,float blue){
 if(car<0||car>=8||!std::isfinite(exposure)||exposure<=0)return;
 auto &d=cars[car];float before=d.amount;
 d.amount=1.f-(1.f-d.amount)*std::exp(-std::min(exposure,.1f));
 float add=d.amount-before;if(add<=0)return;
 float weight=add/std::max(d.amount,.0001f);
 d.red+=(std::clamp(red,0.f,1.f)-d.red)*weight;
 d.green+=(std::clamp(green,0.f,1.f)-d.green)*weight;
 d.blue+=(std::clamp(blue,0.f,1.f)-d.blue)*weight;
}
}
#endif
