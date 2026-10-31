// Matrix projection primitives moved out of FixedPoint.cpp (PORT): the fog
// beam projection (game/FogBeamProjection.h) and its rendered-pixel test
// (tests/fog_beam_render.cpp) need these without the rest of the decompiled
// translation unit. Bodies are unchanged CMR2 code.
#include "FloatToInt.h"
#include "FixedPoint.h"
#include "Graphics.h"
extern const float g_floatHalf;

void FixMatrix_ProjectToScreen(int *pOut, FixVector *pV, int *pM)
{
    float halfH;
    float halfW;
    float x;
    float y;
    float w;
    int z;

    halfH = X87S_MUL(X87S_FILD(float, (int)g_pGraphics->resY), g_floatHalf);
    halfW = X87S_MUL(X87S_FILD(float, (int)g_pGraphics->resX), g_floatHalf);
    x = X87S_MUL(X87S_FILD(float, (FixMul(pM[0], pV->x) + FixMul(pM[4], pV->y) + FixMul(pM[8], pV->z) + pM[12])), CGraphics::m_oneOver65536);
    y = X87S_MUL(X87S_FILD(float, (FixMul(pM[1], pV->x) + FixMul(pM[5], pV->y) + FixMul(pM[9], pV->z) + pM[13])), CGraphics::m_oneOver65536);
    z = FixMul(pM[2], pV->x) + FixMul(pM[6], pV->y) + FixMul(pM[10], pV->z);
    w = (float)(X87S_MUL((int)(FixMul(pM[3], pV->x) + FixMul(pM[7], pV->y) + FixMul(pM[11], pV->z) + pM[15]), CGraphics::m_oneOver65536));
    pOut[0] = (int)FTOI(__int64, X87S_MUL(x / (w / halfW), CGraphics::m_65536));
    pOut[1] = (int)FTOI(__int64, X87S_MUL(-(y / (w / halfH)), CGraphics::m_65536));
}

void FixMatrix_Identity(FixMatrix *pOut)
{
    int m[16] = { 0x10000, 0, 0, 0,
                  0, 0x10000, 0, 0,
                  0, 0, 0x10000, 0,
                  0, 0, 0, 0x10000 };

    *pOut = *(FixMatrix *)m;
}

void FixMatrix_Invert(FixMatrix *pOut, FixMatrix *pIn)
{
    int *m;
    int *o;
    int p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17;
    int c0, c1, c2, c4, c5, c6, c8, c9, c10;
    int det;
    int tx, ty, tz;

    m = (int *)pIn;
    p0 = FixMul(m[5], m[10]);
    p1 = FixMul(m[6], m[9]);
    p2 = FixMul(m[4], m[10]);
    p3 = FixMul(m[6], m[8]);
    p4 = FixMul(m[4], m[9]);
    p5 = FixMul(m[5], m[8]);
    p6 = FixMul(m[1], m[10]);
    p7 = FixMul(m[2], m[9]);
    p8 = FixMul(m[0], m[10]);
    p9 = FixMul(m[2], m[8]);
    p10 = FixMul(m[0], m[9]);
    p11 = FixMul(m[1], m[8]);
    p12 = FixMul(m[1], m[6]);
    p13 = FixMul(m[2], m[5]);
    p14 = FixMul(m[0], m[6]);
    p15 = FixMul(m[2], m[4]);
    p16 = FixMul(m[0], m[5]);
    p17 = FixMul(m[1], m[4]);
    c0 = p0 - p1;
    c1 = p7 - p6;
    c2 = p12 - p13;
    c4 = p3 - p2;
    c5 = p8 - p9;
    c6 = p15 - p14;
    c8 = p4 - p5;
    c9 = p11 - p10;
    c10 = p16 - p17;
    det = FixMul(c0, m[0]) + FixMul(c1, m[4]) + FixMul(c2, m[8]);
    if (det == 0) {
        FixMatrix_Identity(pOut);
        return;
    }
    det = FixDiv(0x10000, det);
    o = (int *)pOut;
    o[11] = 0;
    o[7] = 0;
    o[3] = 0;
    o[0] = FixMul(det, c0);
    o[1] = FixMul(det, c1);
    o[2] = FixMul(det, c2);
    o[4] = FixMul(det, c4);
    o[5] = FixMul(det, c5);
    o[6] = FixMul(det, c6);
    o[8] = FixMul(det, c8);
    o[9] = FixMul(det, c9);
    o[10] = FixMul(det, c10);
    tx = -m[12];
    ty = -m[13];
    tz = -m[14];
    o[12] = FixMul(tx, o[0]) + FixMul(ty, o[4]) + FixMul(tz, o[8]);
    o[13] = FixMul(tx, o[1]) + FixMul(ty, o[5]) + FixMul(tz, o[9]);
    o[14] = FixMul(tx, o[2]) + FixMul(ty, o[6]) + FixMul(tz, o[10]);
    o[15] = 0x10000;
}

void FixMatrix_TransformPoint(FixVector *pOut, FixVector *pIn, FixMatrix *pM)
{
    int x, y, z;

    x = FixMul(pM->right.x, pIn->x) + FixMul(pM->up.x, pIn->y) + FixMul(pM->forward.x, pIn->z) + pM->position.x;
    y = FixMul(pM->right.y, pIn->x) + FixMul(pM->up.y, pIn->y) + FixMul(pM->forward.y, pIn->z) + pM->position.y;
    z = FixMul(pM->right.z, pIn->x) + FixMul(pM->up.z, pIn->y) + FixMul(pM->forward.z, pIn->z) + pM->position.z;
    pOut->x = x;
    pOut->y = y;
    pOut->z = z;
}

void FixMatrix_ProjectWorldPointToView(int *pOut, FixVector *pPoint, SceneNode *pView)
{
    FixVector v;
    FixMatrix inverse;

    FixMatrix_Invert(&inverse, &pView->world);
    FixMatrix_TransformPoint(&v, pPoint, &inverse);
    if (v.z < 0x10000) {
        pOut[0] = -0x640000;
        pOut[1] = -0x640000;
        return;
    }
    FixMatrix_ProjectToScreen(pOut, &v, CGraphics::m_pTextureManager->fixedProjection);
    pOut[0] += (int)FTOI(__int64, X87S_MUL((g_pGraphics->resX / 2), CGraphics::m_65536));
    pOut[1] += (int)FTOI(__int64, X87S_MUL((g_pGraphics->resY / 2), CGraphics::m_65536));
}

