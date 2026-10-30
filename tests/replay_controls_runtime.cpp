#include "port/types.h"
#include "platform/testing.h"
#include "ReplayStream.h"
#include "ReplayControls.h"
#include <cstdio>
extern BYTE *g_playerReplayStreams[8];
void ReplayControls_Update();float ReplayControls_TimeScale();bool ReplayControls_KeyIsReplayKey(int);
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 ReplayStream stream={};stream.playing=1;g_playerReplayStreams[0]=reinterpret_cast<BYTE*>(&stream);
 BYTE keys[256]={};Platform_SetTestKeyboard(keys);
 ReplayControls_Update();CHECK(ReplayControls_TimeScale()==1);
 keys[kReplayKeyPause]=0x80;ReplayControls_Update();CHECK(ReplayControls_TimeScale()==0);
 ReplayControls_Update();CHECK(ReplayControls_TimeScale()==0); // hold is not a second press
 keys[kReplayKeyPause]=0;ReplayControls_Update();keys[kReplayKeyPause]=0x80;ReplayControls_Update();CHECK(ReplayControls_TimeScale()==1);
 CHECK(ReplayControls_KeyIsReplayKey(kReplayKeyPause));
 stream.playing=0;ReplayControls_Update();CHECK(ReplayControls_TimeScale()==1&&!ReplayControls_KeyIsReplayKey(kReplayKeyPause));
 Platform_SetTestKeyboard(nullptr);g_playerReplayStreams[0]=nullptr;
 puts("actual raw Space replay pause/hold/release/resume/reset passes");
}
