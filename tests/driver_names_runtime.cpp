#include "port/types.h"
#include "port/sys.h"
#include "AIHelper.h"
#include "GameInfo.h"
#include <cstring>
#include <string>
#include <cstdarg>
#include <cstdio>
static std::string mode,payload,logs;static int reads;
extern "C" void __wrap_Sys_Log(const char *fmt,...){char out[1024];va_list args;va_start(args,fmt);vsnprintf(out,sizeof out,fmt,args);va_end(args);logs+=out;}
extern "C" int __wrap_Sys_GetOption(const char*,int){return mode=="disabled"?0:1;}
extern "C" SysFile *__wrap_Sys_OpenFile(const char *name,int){reads++;return mode=="missing"||strcmp(name,"driver-names.tsv")?nullptr:reinterpret_cast<SysFile*>(1);}
extern "C" DWORD __wrap_Sys_GetFileSize(SysFile*){return payload.size();}
extern "C" DWORD __wrap_Sys_ReadFile(SysFile*,void* dst,DWORD n){memcpy(dst,payload.data(),n);return n;}
extern "C" void __wrap_Sys_CloseFile(SysFile*){}
int main(int argc,char **argv){
 mode=argc>1?argv[1]:"valid";payload="OPENCMR2-DRIVER-NAMES-1\n";
 for(int i=0;i<15;i++)payload+=std::to_string(i)+"\tpatched "+std::to_string(i)+"\n";
 if(mode=="invalid")payload+="extra\n";
 CGameInfo::m_gameInfo.field_0x14=0;
 const char *expected=mode=="valid"?"patched 14":"utzman";
 if(strcmp(CAIHelper::GetNameForID(14),expected))return 1;
 if(strcmp(CAIHelper::GetNameForID(15),mode=="valid"?"patched 0":"mcrae"))return 2;
 CAIHelper::LogDriverNameStatus();
 if(logs.find(mode=="valid"?"custom (15 validated names loaded)":mode=="disabled"?"original (disabled)":mode=="missing"?"original (sidecar missing)":"original (invalid sidecar)")==std::string::npos)return 4;
 if(reads!=(mode=="disabled"?0:1))return 3;
 return 0;
}
