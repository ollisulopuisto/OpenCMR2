#include "platform/platform.h"
#include "port/sys.h"
#include <cstdio>
#include <cstring>
int main(int argc,char**argv){
 if(!Platform_Init(argc,argv))return 1;
 char text[32]={};SysFile *f=Sys_OpenFile("Game/Cars/SUB.CIN",SYS_FILE_READ);
 if(!f)return 2;Sys_ReadFile(f,text,sizeof text-1);Sys_CloseFile(f);
 if(strcmp(text,"replacement car"))return 3;
 f=Sys_OpenFile("pps/progress.pps",SYS_FILE_READ);if(!f)return 4;
 memset(text,0,sizeof text);Sys_ReadFile(f,text,sizeof text-1);Sys_CloseFile(f);if(strcmp(text,"valuable profile"))return 5;
 f=Sys_OpenFile("Game/Cars/sub.cin",SYS_FILE_WRITE);if(!f)return 6;
 Sys_WriteFile(f,"user write",10);Sys_CloseFile(f);
 // Read overlay remains read-only and unchanged even for a similarly named user write.
 f=Sys_OpenFile("Game/Cars/sub.cin",SYS_FILE_READ);if(!f)return 7;
 memset(text,0,sizeof text);Sys_ReadFile(f,text,sizeof text-1);Sys_CloseFile(f);if(strcmp(text,"replacement car"))return 8;
 Platform_Shutdown();return 0;
}
