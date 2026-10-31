#include "port/types.h"
#include "platform/testing.h"
#include "ReplayStream.h"
#include "CameraRecord.h"
#include "SceneNode.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>
extern BYTE *g_playerReplayStreams[8];extern CameraRecord g_viewRecords[4],g_viewStates[2],g_viewPreviousStates[2];extern BYTE g_activeViewRecord[4],g_droneVariant[8];extern SceneNode*g_viewNodes[3];
void View_ReplayChaseFrame(unsigned,int,int,int,int,int);void View_ReplayChaseReset();void View_BlendCameraStates(BYTE,int);bool ReplayControls_KeyIsFreeCamKey(int);void ReplayControls_Update();
static double ms=1000;static DWORD tick(){return(DWORD)ms;}static double precise(){return ms;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
void Scene_InitFixedMathTables();
int main(){
 Scene_InitFixedMathTables();
 PlatformTestClock clock={tick,nullptr,0,precise};Platform_SetTestClock(&clock);
 ReplayStream stream={};stream.playing=1;g_playerReplayStreams[0]=reinterpret_cast<BYTE*>(&stream);
 SceneNode nodes[2]={};g_viewNodes[0]=nodes;g_viewNodes[1]=nodes+1;
 for(int view=0;view<2;view++){
  CameraRecord c={};c.matrix.right.x=c.matrix.up.y=c.matrix.forward.z=65536;c.matrix.position.z=-8*65536;c.lookDistance=8*65536;c.nearPlane=6553;
  g_viewStates[view]=g_viewPreviousStates[view]=c;
 }
 for(int view=0;view<2;view++)for(int slot=0;slot<2;slot++)for(int mode:{4,5,6}){
  g_activeViewRecord[view]=slot;g_viewRecords[view*2+slot].type=mode;
  View_ReplayChaseReset();View_ReplayChaseFrame(view,0,0,0,0,0);ms+=100;View_ReplayChaseFrame(view,0,1,1,0,0);
  CameraRecord before=g_viewStates[view];View_BlendCameraStates(view,65536);if(view==0&&slot==0&&mode==4){auto&m=nodes[view].current;fprintf(stdout,"pose %d %d %d %d %d %d %d %d %d %d %d %d\n",m.position.x,m.position.y,m.position.z,m.right.x,m.right.y,m.right.z,m.up.x,m.up.y,m.up.z,m.forward.x,m.forward.y,m.forward.z);}CHECK(nodes[view].current.position.x!=0);CHECK(!memcmp(&before,&g_viewStates[view],sizeof before));
  FixMatrix first=nodes[view].current;View_BlendCameraStates(view,65536);CHECK(!memcmp(&first,&nodes[view].current,sizeof first)); // never accumulates into original
  View_ReplayChaseFrame(view,0,0,0,0,1);View_BlendCameraStates(view,65536);CHECK(nodes[view].current.position.x==0&&nodes[view].current.position.z==-8*65536); // exact active default
 }
 g_activeViewRecord[0]=0;g_viewRecords[0].type=5;g_droneVariant[0]=1;CHECK(ReplayControls_KeyIsFreeCamKey(0xc8)&&ReplayControls_KeyIsFreeCamKey(0x13)&&!ReplayControls_KeyIsFreeCamKey(0x11));
 g_viewRecords[0].type=3;CHECK(!ReplayControls_KeyIsFreeCamKey(0xc8));g_viewRecords[0].type=4;
 BYTE keys[256]={};Platform_SetTestKeyboard(keys);keys[0x39]=0x80;ReplayControls_Update();keys[0x39]=0;ReplayControls_Update();
 View_ReplayChaseReset();ReplayControls_Update();ms+=100;keys[0xcd]=0x80;ReplayControls_Update();View_BlendCameraStates(0,65536);CHECK(nodes[0].current.position.x!=0);keys[0xcd]=0;keys[0x13]=0x80;ReplayControls_Update();View_BlendCameraStates(0,65536);CHECK(nodes[0].current.position.x==0);Platform_SetTestKeyboard(nullptr);
 stream.playing=0;ReplayControls_Update();CHECK(!ReplayControls_KeyIsFreeCamKey(0x13));View_BlendCameraStates(0,65536);CHECK(nodes[0].current.position.x==0);
 g_playerReplayStreams[0]=nullptr;g_viewNodes[0]=g_viewNodes[1]=nullptr;Platform_SetTestClock(nullptr);
 puts("actual render blend: both view slots, chase4/5/6, no record mutation/accumulation, exact R reset, replay-only key routing pass");
}
