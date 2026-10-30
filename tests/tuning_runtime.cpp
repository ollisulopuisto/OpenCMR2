#include "port/types.h"
#include "platform/testing.h"
#include "port/sys.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
void TuningPanel_Update();bool TuningPanel_IsOpen();bool TuningPanel_InfoShown();bool TuningPanel_FpsOnly();bool TuningPanel_PreviewActive();
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 auto old=std::filesystem::current_path();auto tmp=std::filesystem::temp_directory_path()/"opencmr-tuning-runtime";
 std::filesystem::create_directories(tmp);std::filesystem::current_path(tmp);std::ofstream("opencmr2.ini")<<"[video]\n";
 BYTE keys[256]={};Platform_SetTestKeyboard(keys);TuningPanel_Update();
 CHECK(TuningPanel_InfoShown()&&!TuningPanel_IsOpen()&&!TuningPanel_FpsOnly());
 for(int lap=0;lap<2;lap++)for(int mode=1;mode<=4;mode++){
  keys[68]=0x80;TuningPanel_Update();int m=mode%4;
  CHECK(TuningPanel_IsOpen()==(m==1));CHECK(TuningPanel_FpsOnly()==(m==3));CHECK(TuningPanel_InfoShown()==(m!=2));CHECK(TuningPanel_PreviewActive());
  TuningPanel_Update();CHECK(TuningPanel_IsOpen()==(m==1)&&TuningPanel_FpsOnly()==(m==3));
  keys[68]=0;TuningPanel_Update();CHECK(TuningPanel_IsOpen()==(m==1)&&TuningPanel_FpsOnly()==(m==3));
 }
 Platform_SetTestKeyboard(nullptr);std::filesystem::current_path(old);std::filesystem::remove_all(tmp);
 puts("actual F10 raw input: four modes, hold/release, repeat laps, preview persistence pass");
}
