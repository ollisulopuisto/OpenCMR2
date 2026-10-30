#include "port/types.h"
#include "port/msvcrt.h"
#include "France1Textures.h"
#include "Texture.h"
#include "Graphics.h"
#include <map>
#include <set>
#include "ScenerySHA256.h"
#include "GenericFileLoader.h"
#include "port/sys.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>
#include <sstream>
namespace {
bool active=false,uprez=false;
std::map<Texture*,GfxTexture*> originals;
std::map<Texture*,GfxTexture*> verifiedCutouts;
struct Source{const char *hash;const char *file;const char *output;};
const Source sources[]={
{"a57557d985545afa97514b5aa331484d77f30b666d2bf4e5e27f55755197c3ef","t_1a-frahit.rgba","051308909c3ca51c24e4928010967a7b65acf3cd3bafd88c31b3edd0ca38e1f8"},
{"662720d7930feaf071d6082a63cecf0d3fd5aebbdbebf7ce6de3d351f2a0393c","t_1b-frahit.rgba","3f1eb6dcaa51a8b937f48234bcd2ee38a252d2677930e772ff51b24b9990ec32"},
{"a78262840120f54a633e4e1896660da1065d0b70b58b0c246664f30f99087e98","t_1a-fralot.rgba","b3516788fd0b1298f61b79735a3213edbe33257b4abef7f08776bd30ed134082"},
{"91cac6cd95bbe2a726a19185d894732fa8553afa095a44bf6385a76ea91c4690","t_1b-fralot.rgba","37b14fe40da04bc88cb023c3d073ed275c1e5d95c9d919411f9b47654c41c522"}};
#include "UprezSources.inc"
bool Read(const std::filesystem::path&p,std::vector<char>&b,unsigned limit){
 std::error_code ec;if(std::filesystem::is_symlink(p,ec)||!std::filesystem::is_regular_file(p,ec))return false;
 auto n=std::filesystem::file_size(p,ec);if(ec||!n||n>limit)return false;
 b.resize(n);std::ifstream f(p,std::ios::binary);return bool(f.read(b.data(),n));
}
unsigned U32(const std::vector<char>&b,unsigned p){return (unsigned char)b[p]|((unsigned char)b[p+1]<<8)|((unsigned char)b[p+2]<<16)|((unsigned char)b[p+3]<<24);}
}
void France1Textures_Clear(){
 active=false;
 // Check current live records before dereferencing a retained pointer. Texture
 // allocations may have been released/reused during stage unwind.
 for(unsigned i=0;i<CGraphics::m_textureCount;i++){
  Texture*t=CGraphics::m_pTextureManager->textureBuffer[i];auto found=originals.find(t);
  if(found!=originals.end() && t && t->pSurface==found->second)Gfx_SetTextureReplacement(t->pSurface,nullptr);
 }
 originals.clear();verifiedCutouts.clear();
}
void France1Textures_Begin(GenericFile *archive,const char *stage){
 France1Textures_Clear();
 if(!stage||!Sys_GetTextureDirectory()||!*Sys_GetTextureDirectory())return;
 uprez=Sys_GetOption("video.texture_uprez",0)!=0;
 if(_stricmp(stage,"fra01")&&(!uprez||_stricmp(stage,"fin01")))return;
 DWORD n=0;const void*p=CGenericFileLoader::FindFile(archive,(char*)"temp.c3d",nullptr,&n,0);
 if(!p||n>4*1024*1024)return;
 auto hash=SceneryIdentity::SHA256(p,n);
 active=hash=="a78fbf25efcecb20a0cd7360c2bfc28e24ba040a028f6225a0d1f690f9eed8d0"||hash=="f5331c231abc299df2b2a42c9b0c2bb8e69ac79526a5c21f5550ed7311157c36";
 if(uprez&&!_stricmp(stage,"fin01"))active=hash=="ecfd29aac42deb799d798fa8ce2a5db840147f77a3d49c80276f93d0e8a3157b"||hash=="b613c03d3c3cd7e0e6cf14c99be09e9ace96863af520e350f0cf215176a89c35";
 Sys_Log("textures: exact-source stage texture test %s; source2x filter, no new detail; native unverified",active?"source verified":"refused");
}
bool France1Textures_Apply(Texture *texture,const void*bytes,unsigned size){
 verifiedCutouts.erase(texture);
 if(!active||!texture||!texture->pSurface||!bytes||(size<128||size>1024*1024))return false;
 auto hash=SceneryIdentity::SHA256(bytes,size);const Source*source=nullptr;
 if(uprez){for(auto&s:uprezSources)if(hash==s.hash){source=&s;break;}}
 else for(auto&s:sources)if(hash==s.hash){source=&s;break;}if(!source)return false;
 std::filesystem::path root(Sys_GetTextureDirectory());std::vector<char> manifest,raw;
 if(!Read(root/(uprez?"uprez-textures.tsv":"france1-textures.tsv"),manifest,131072))return false;
 std::istringstream in(std::string(manifest.begin(),manifest.end()));std::string line;if(!std::getline(in,line)||line!=(uprez?"OPENCMR2-UPREZ-TEXTURES-1":"OPENCMR2-FRANCE1-TEXTURES-1"))return false;
 std::string expected;while(std::getline(in,line)){
  std::istringstream row(line);std::string h,file,digest;if(!std::getline(row,h,'\t')||!std::getline(row,file,'\t')||!std::getline(row,digest))return false;
  if(h==hash&&file==source->file){if(!expected.empty())return false;expected=digest;}
 }
 if(expected!=source->output||!Read(root/source->file,raw,2*1024*1024)||SceneryIdentity::SHA256(raw.data(),raw.size())!=expected||raw.size()<20||memcmp(raw.data(),"OCMRRGBA",8))return false;
 if(size<128||memcmp(bytes,"DDS ",4))return false;
 const auto*dds=(const unsigned char*)bytes;
 auto dim=[&](unsigned p){return unsigned(dds[p])|(unsigned(dds[p+1])<<8)|(unsigned(dds[p+2])<<16)|(unsigned(dds[p+3])<<24);};
 unsigned w=U32(raw,8),h=U32(raw,12),levels=U32(raw,16);
 if(!dim(12)||!dim(16)||dim(12)>1024||dim(16)>1024||w!=dim(16)*2||h!=dim(12)*2)return false;
 unsigned wantLevels=1;for(unsigned a=w,b=h;a>1||b>1;a=std::max(1u,a/2),b=std::max(1u,b/2))++wantLevels;
 if(levels!=wantLevels)return false;
 size_t total=20;unsigned a=w,b=h;for(unsigned i=0;i<levels;i++){total+=a*b*4;a=std::max(1u,a/2);b=std::max(1u,b/2);}if(total!=raw.size())return false;
 GfxTexture *replacement=Gfx_CreateTexture(w,h,GFX_FORMAT_BGRA8,levels,0);if(!replacement)return false;
 size_t offset=20;a=w;b=h;
 for(unsigned i=0;i<levels;i++){
  GfxLockedRect lock={};if(!Gfx_LockTexture(replacement,0,i,&lock)){Gfx_DestroyTexture(replacement);return false;}
  if(lock.pitch<int(a*4)){Gfx_UnlockTexture(replacement,0,i);Gfx_DestroyTexture(replacement);return false;}
  for(unsigned y=0;y<b;y++){
   auto*dst=(unsigned char*)lock.pixels+y*lock.pitch;auto*src=(const unsigned char*)raw.data()+offset+y*a*4;
   for(unsigned x=0;x<a;x++){dst[x*4]=src[x*4+2];dst[x*4+1]=src[x*4+1];dst[x*4+2]=src[x*4];dst[x*4+3]=src[x*4+3];}
  }
  Gfx_UnlockTexture(replacement,0,i);offset+=a*b*4;a=std::max(1u,a/2);b=std::max(1u,b/2);
 }
 originals[texture]=texture->pSurface;
 if(uprez){std::string file=source->file;
  if(file.find("tresh")==0||file.find("tre_w2")==0||file.find("tre_wl")==0||file.find("bsh_st1")==0)verifiedCutouts[texture]=texture->pSurface;
 }
 Gfx_SetTextureReplacement(texture->pSurface,replacement); // original dimensions/UVs survive rollback
 Sys_Log("textures: source2x applied %s (%ux%u, %u mips)",source->file,w,h,levels);return true;
}

bool France1Textures_IsVerifiedCutout(Texture*t){auto i=verifiedCutouts.find(t);return active&&uprez&&i!=verifiedCutouts.end()&&t&&t->pSurface==i->second;}
