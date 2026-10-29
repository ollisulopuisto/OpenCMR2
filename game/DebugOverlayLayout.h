#ifndef OPENCMR2_DEBUG_OVERLAY_LAYOUT_H
#define OPENCMR2_DEBUG_OVERLAY_LAYOUT_H
#pragma push_macro("rand")
#undef rand
#include <algorithm>
#pragma pop_macro("rand")
struct DebugOverlayLayout {
 int x,y,width,height,font,line,pad;
 static DebugOverlayLayout For(int width,int height,int lines) {
  // Logical text height scales with render resolution, with a readable floor.
  int font=std::clamp(std::min(width/100,height/70),14,32);
  int pad=std::max(6,font/2),margin=std::max(6,height/160),line=font+std::max(4,font/4);
  return {margin,margin,std::min(width-2*margin,font*(lines>4?40:27)+2*pad),lines*line+2*pad,font,line,pad};
 }
};
#endif
