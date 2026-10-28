#ifndef OPENCMR2_CROWD_SELECTION_H
#define OPENCMR2_CROWD_SELECTION_H
#include <cstdint>
namespace CrowdSelection {
// Stable placement selection, shared by high/low variants. No physics RNG.
inline bool Articulate(int32_t x,int32_t z){
 uint32_t seed=(uint32_t(x)*1664525u)^uint32_t(z);
 seed^=seed>>16;seed*=0x7feb352du;seed^=seed>>15;
 return seed%100u<65u;
}
}
#endif
