#include "CameraOperator.h"
#include "CameraRecord.h"
#include "Car.h"
#include "port/sys.h"
static CameraOperator::State states[8];
extern BYTE g_playerCameraMode[8];
bool CameraOperator_Enabled(const CameraRecord*record){
 return record && record->index<8 && g_playerCameraMode[record->index]==4 && Sys_GetOption("video.camera_operator",0);
}
void CameraOperator_Reset(unsigned i){if(i<8)states[i].Reset();}
void CameraOperator_Update(CameraRecord*record,const FixMatrix*reference){
 if(!record||record->index>=8)return;auto&state=states[record->index];
 if(!reference||!CameraOperator_Enabled(record)||record->car>=8){state.Reset();return;}
 Car*car=Car_Get(record->car);if(!car){state.Reset();return;}
 if(!car->pWorld){state.Reset();return;}
 // Original chase forward is the reference right axis (ChaseCamera_Restart),
 // not the matrix field named forward. Match that game coordinate convention.
 float x=car->pWorld->right.x/65536.f,z=car->pWorld->right.z/65536.f;
 float vx=car->velocity.x/65536.f,vz=car->velocity.z/65536.f;
 if(!std::isfinite(x)||!std::isfinite(z)||x*x+z*z<.25f){state.Reset();return;}
 float heading=std::atan2(x,z),travel=vx*vx+vz*vz>.0001f?std::atan2(vx,vz):heading;
 state.Step(heading,travel,car->speed>0?1.f:0.f,g_physicsTimeStep/(65536.f*25.f));
}
void CameraOperator_Apply(CameraRecord*record){
 if(!record||record->index>=8||!CameraOperator_Enabled(record))return;
 float a=states[record->index].angle,c=std::cos(a),s=std::sin(a);auto&m=record->matrix;FixVector r=m.right,f=m.forward;
 // Reframe orientation only; preserve custom camera's position/height/distance,
 // pitch/roll and original terrain-clearance/collision machinery.
 m.right.x=std::lround(r.x*c-f.x*s);m.right.y=std::lround(r.y*c-f.y*s);m.right.z=std::lround(r.z*c-f.z*s);
 m.forward.x=std::lround(f.x*c+r.x*s);m.forward.y=std::lround(f.y*c+r.y*s);m.forward.z=std::lround(f.z*c+r.z*s);
}
