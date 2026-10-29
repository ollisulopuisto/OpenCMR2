#pragma once
#include "gfx_internal.h"
// Only a supported fixed-function alpha test is reproduced. Translucent
// glass/smoke, generated coordinates and multi-stage alpha stay excluded.
inline bool Gfx_CutoutShadow(const GfxDrawState &s) {
 const auto &a=s.stages[0];
 return s.alphaTest && (!s.alphaBlend || (s.srcBlend==GFX_BLEND_SRCALPHA && s.destBlend==GFX_BLEND_INVSRCALPHA)) && a.texture && !(a.texture->flags&GFX_TEXTURE_CUBE) &&
  (s.alphaFunc==GFX_CMP_GREATER || s.alphaFunc==GFX_CMP_GREATEREQUAL) &&
  a.alphaArg1==GFX_TA_TEXTURE &&
  (a.alphaOp==GFX_TOP_SELECTARG1 || (a.alphaOp==GFX_TOP_MODULATE && a.alphaArg2==GFX_TA_DIFFUSE)) &&
  a.texCoordIndex==0 && a.textureTransform==GFX_TTFF_DISABLE &&
  s.stages[1].colorOp==GFX_TOP_DISABLE;
}
inline bool Gfx_ShadowCaster(const GfxDrawState &s) {
 return s.depthTest && s.depthWrite && ((!s.alphaBlend&&!s.alphaTest)||Gfx_CutoutShadow(s));
}
