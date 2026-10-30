#include "port/types.h"
#include "CurvedRoad.h"
#include "ScenerySHA256.h"
#include "GenericFileLoader.h"
#include "FileBuffer.h"
#include "Game.h"
#include "port/sys.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>
extern void Track_ResearchExactEdges(bool);
namespace {
void *render=nullptr,*contact=nullptr;
bool selected=false;
void Clear(){
 selected=false;Track_ResearchExactEdges(false);
 CFileBuffer::FreeGenericFileBuffer(render);CFileBuffer::FreeGenericFileBuffer(contact);
 render=contact=nullptr;
}
bool Original(GenericFile*a,const char*name,const char*hash){
 DWORD n=0;auto*p=CGenericFileLoader::FindFile(a,const_cast<char*>(name),nullptr,&n,0);
 return p&&n<16*1024*1024&&SceneryIdentity::SHA256(p,n)==hash;
}
bool Read(const std::filesystem::path &path,const char*hash,std::vector<char>&out){
 std::error_code ec;if(std::filesystem::is_symlink(path,ec)||!std::filesystem::is_regular_file(path,ec))return false;
 auto n=std::filesystem::file_size(path,ec);if(ec||n==0||n>16*1024*1024)return false;
 std::ifstream f(path,std::ios::binary);out.resize(n);f.read(out.data(),n);
 return f&&SceneryIdentity::SHA256(out.data(),out.size())==hash;
}
}
void CurvedRoad_Begin(GenericFile *a,const char*stage){
 selected=false;Track_ResearchExactEdges(false);
 // Prior session must have been released by the stage's LIFO callbacks.
 // Never free an active render attachment while its sector parts still exist.
 if(render||contact){Sys_Log("roads: active session not released; keeping original new stage");return;}
 const char*dir=Sys_GetRoadDirectory();if(!dir||!*dir)return;
 const bool fin=stage&&!_stricmp(stage,"fin01");
 const bool fra=stage&&!_stricmp(stage,"fra01");
 bool hi=false,lo=false;
 if(fin){hi=Original(a,"temp.c3d","ecfd29aac42deb799d798fa8ce2a5db840147f77a3d49c80276f93d0e8a3157b");lo=Original(a,"temp.c3d","b613c03d3c3cd7e0e6cf14c99be09e9ace96863af520e350f0cf215176a89c35");}
 const bool identity=fin?((hi||lo)&&
 Original(a,"fin01.hpc","caf5e507b037c67d1c62819256c63dcb355733979240c959b4487803fc02c2a0")&&
 Original(a,"fin01fin.dat","0f1efdc2d7ad5e7fe9ddeefed0c6fdba5cf4b6086fcda452a675837edfc2761d")&&
 Original(a,"fin01.srf","799256fe97aac42b5b7a197fd3b03af4cf4065e4141ba05ad91eb1cbfb572adb")):
 (fra&&Original(a,"temp.c3d","a78fbf25efcecb20a0cd7360c2bfc28e24ba040a028f6225a0d1f690f9eed8d0")&&
 Original(a,"fra01.hpc","2d262119d5b768b2517dc7c2b7f66968bfe1a5becf0964e9ed2691316040fa3e")&&
 Original(a,"fra01fin.dat","5e99cc9e3f9eced8ee6f27de89cd79db0eeae0b30ca01463b06ebf9f0fbf4532")&&
 Original(a,"fra01.srf","d087dd073fc11f11f35886cc2d436d0228ae834c1678a9f5e0c6363ca0b80565"));
 if(!identity){Sys_Log("roads: unsupported stage/source; BOTH original render and contact retained");return;}
 std::vector<char>r,h;
 const char*rf=fin?(hi?"hi-curved.c3d":"lo-curved.c3d"):"curved.c3d";
 const char*rh=fin?(hi?"e922df379b293a31f5f96de4ee9d58b6c854b485e6a776d50d3181a4fb4be94d":"2f932e4b3a6e94b4490c2afab111b809bd5ba402edca20915b17a211efacd001"):"b694283293c0f9bbfbfcd1dbeaf3fba974dd23d632fc5d2d7dbf7e39a0c617a7";
 const char*hf=fin?(hi?"hi-curved.hpc":"lo-curved.hpc"):"curved.hpc";
 const char*hh=fin?"d4f44db7e6fe2e8ff6c85a262af5890b836c42263cb4a06f8df9b895a5c0bd3a":"5b051ddd0cc7749e1ef70f7e8da902ade7fbe039ee82cf8edc877ad09f6d5a18";
 if(!Read(std::filesystem::path(dir)/rf,rh,r)||(!fin&&!Read(std::filesystem::path(dir)/hf,hh,h))){
 Sys_Log("roads: missing/changed pair; BOTH originals retained");return;}
 auto*rp=CFileBuffer::AllocateLockedBuffer(r.size());auto*hp=fin?nullptr:CFileBuffer::AllocateLockedBuffer(h.size());
 if(!rp||(!fin&&!hp)){CFileBuffer::FreeGenericFileBuffer(rp);CFileBuffer::FreeGenericFileBuffer(hp);Sys_Log("roads: pair allocation failed; BOTH originals retained");return;}
 if(CGame::RegisterCallback((void*)Clear,nullptr)<0){CFileBuffer::FreeGenericFileBuffer(rp);CFileBuffer::FreeGenericFileBuffer(hp);Sys_Log("roads: no lifecycle callback slot; BOTH originals retained");return;}
 memcpy(rp,r.data(),r.size());if(hp)memcpy(hp,h.data(),h.size());render=rp;contact=hp;selected=true;
 // Registered before mesh/sector callbacks, released after them by LIFO unwind.
 Track_ResearchExactEdges(!fin);
 Sys_Log(fin?"roads: Finland1 VISUAL-ONLY render active; ORIGINAL contact/FIN/route/physics preserved; native appearance unverified":"roads: experimental Ballon paired render/contact; FIN unchanged; native handling unverified");
}
void *CurvedRoad_Render(){return selected?render:nullptr;}
void *CurvedRoad_Contact(){return selected?contact:nullptr;}
