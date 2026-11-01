#pragma once
// Presentation-only rigid rotation about the current chase camera's aim.
// Apply to a fresh interpolated pose each render, never to simulation records.
#include <cmath>
#include <initializer_list>
struct ReplayChaseOrbit {
    double yaw=0, pitch=0;
    void Reset(){yaw=pitch=0;}
    static bool Chase(int mode){return mode>=4 && mode<=6;}
    // When are the orbit keys captured by something else? Only while the race
    // menu or the tuning ADJUST panel is open. The tuning "engaged" preview
    // flag is sticky after the first F10 press, so gating on it killed the
    // arrows for the rest of the session (user report: replay chase arrows
    // dead while the camera readout was on screen).
    static bool InputCaptured(bool menuOpen, bool tuningPanelOpen){return menuOpen||tuningPanelOpen;}
    static bool Key(int key){return key==0xc8||key==0xd0||key==0xcb||key==0xcd||key==0x13;}
    void Step(bool left,bool right,bool up,bool down,double dt){
        if(!(dt>0)||!std::isfinite(dt))return;
        if(dt>.1)dt=.1;
        yaw=std::remainder(yaw+(int(right)-int(left))*1.4*dt,6.283185307179586);
        pitch+=(int(up)-int(down))*1.4*dt;
        if(pitch>1.3)pitch=1.3;if(pitch< -1.3)pitch= -1.3;
    }
    // Rodrigues rotation. Handedness is measured from the input camera axes.
    static void Rotate(double v[3],const double axis[3],double a){
        double c=std::cos(a),s=std::sin(a),d=v[0]*axis[0]+v[1]*axis[1]+v[2]*axis[2];
        double cross[3]={axis[1]*v[2]-axis[2]*v[1],axis[2]*v[0]-axis[0]*v[2],axis[0]*v[1]-axis[1]*v[0]};
        for(int i=0;i<3;i++)v[i]=v[i]*c+cross[i]*s+axis[i]*d*(1-c);
    }
    void Apply(double pos[3],double right[3],double up[3],double forward[3],double distance)const{
        if(yaw==0&&pitch==0)return;
        double aim[3],delta[3];for(int i=0;i<3;i++){aim[i]=pos[i]+forward[i]*distance;delta[i]=pos[i]-aim[i];}
        double hand=(right[0]*forward[2]-right[2]*forward[0])<0?-1:1;
        const double worldUp[3]={0,1,0};
        for(double*v:{delta,right,up,forward})Rotate(v,worldUp,yaw*hand);
        double axis[3]={right[0],right[1],right[2]};double n=std::sqrt(axis[0]*axis[0]+axis[1]*axis[1]+axis[2]*axis[2]);if(!(n>.1))return;
        for(double&v:axis)v/=n;
        for(double*v:{delta,up,forward})Rotate(v,axis,pitch*hand);
        for(int i=0;i<3;i++)pos[i]=aim[i]+delta[i];
    }
};
