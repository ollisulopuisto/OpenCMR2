#pragma once
#include "dust_volume_math.h"
#include "temporal_history.h"
#include <cmath>
#include <cstring>
// Per-frame camera bookkeeping for the opt-in temporal AA resolve. The world
// camera comes from the WORLD_END command's draw state, like the cloud pass.
struct TemporalUniforms {float inverse[16],previous[16],params[4],target[4];};
struct TemporalFrame {
    bool active=false, hasHistory=false;
    float jitterX=0, jitterY=0; // pixels, applied to world 3D draws only
    float vp[16]={}, eye[3]={}, forward[3]={};
    TemporalUniforms u={};
};
class TemporalTracker {
    float prevVP[16]={}, prevEye[3]={}, prevForward[3]={};
    bool have=false;
    int w=0, h=0;
    unsigned sequence=0;
public:
    void Reset(){have=false;}
    // False means: no temporal this frame (history is dropped).
    bool Prepare(const GfxCommand &c,const GfxDrawState &s,int bw,int bh,TemporalFrame &f){
        f=TemporalFrame();
        if(!c.raceWorld||s.projection._34==0){Reset();return false;}
        int sw=s.screenW?s.screenW:bw,sh=s.screenH?s.screenH:bh;
        if(s.viewport.x||s.viewport.y||s.viewport.width!=(DWORD)sw||s.viewport.height!=(DWORD)sh){Reset();return false;}
        GfxMatrix vp;
        for(int r=0;r<4;r++)for(int col=0;col<4;col++){(&vp._11)[r*4+col]=0;for(int k=0;k<4;k++)(&vp._11)[r*4+col]+=(&s.view._11)[r*4+k]*(&s.projection._11)[k*4+col];}
        float inverseView[16];
        if(!DustVolume_Inverse(vp,f.u.inverse)||!DustVolume_Inverse(s.view,inverseView)){Reset();return false;}
        for(float v:f.u.inverse)if(!std::isfinite(v)){Reset();return false;}
        memcpy(f.vp,&vp._11,sizeof(f.vp));
        f.eye[0]=inverseView[12];f.eye[1]=inverseView[13];f.eye[2]=inverseView[14];
        float fl=std::sqrt(inverseView[8]*inverseView[8]+inverseView[9]*inverseView[9]+inverseView[10]*inverseView[10]);
        if(fl<1e-6f){Reset();return false;}
        for(int i=0;i<3;i++)f.forward[i]=inverseView[8+i]/fl;
        if(w!=bw||h!=bh){Reset();w=bw;h=bh;}
        if(have){
            float dx=f.eye[0]-prevEye[0],dy=f.eye[1]-prevEye[1],dz=f.eye[2]-prevEye[2];
            float dot=f.forward[0]*prevForward[0]+f.forward[1]*prevForward[1]+f.forward[2]*prevForward[2];
            // Camera cut: teleport or sharp turn. Thresholds are guesses; unverified natively.
            if(dx*dx+dy*dy+dz*dz>200.f*200.f||dot<0.7f)have=false;
        }
        Temporal::Sample j=Temporal::Jitter(sequence);
        f.jitterX=j.x;f.jitterY=j.y;
        f.hasHistory=have;
        memcpy(f.u.previous,have?prevVP:f.vp,sizeof(f.u.previous));
        f.u.params[0]=have?0.9f:0.f;f.u.params[1]=0.02f;f.u.params[2]=f.u.params[3]=0.f;
        f.u.target[0]=1.f/bw;f.u.target[1]=1.f/bh;f.u.target[2]=f.u.target[3]=0.f;
        f.active=true;
        return true;
    }
    // Only after the resolve was submitted; a failed frame must not become history.
    void Commit(const TemporalFrame &f){
        memcpy(prevVP,f.vp,sizeof(prevVP));memcpy(prevEye,f.eye,sizeof(prevEye));memcpy(prevForward,f.forward,sizeof(prevForward));
        have=true;sequence++;
    }
};
