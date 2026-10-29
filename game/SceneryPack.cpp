#include "SceneryMotion.h"
#include "SceneryPack.h"
#include "CrowdSelection.h"
#include "ScenerySHA256.h"
#include "SceneryPackFormat.h"
#include "ScenerySourceFit.h"
#include "Graphics.h"
#include "Texture.h"
#include "Mesh.h"
#include "MeshVertex.h"
#include "Sector.h"
#include "port/sys.h"
#include "port/gfx.h"
#include <map>
#include <set>
#ifdef rand
#undef rand
#endif
#include <algorithm>
#include <cstring>
#include <limits>
using namespace SceneryFormat;
namespace {
std::map<GenericFile*,std::string> archives;
std::map<const Texture*,std::string> sourceIdentities;
struct Identity {std::string archive,source,hash;};
std::vector<Identity> identities;
std::map<std::string,std::set<std::string>> windIdentities;
std::set<std::string> windLogged,treeOriginalLogged;
std::set<std::string> unmatched;
struct PackTexture { GfxTexture *surface=nullptr; int width=0,height=0; };
std::vector<GfxTexture*> ownedTextures;
uint64_t animationTime=0;
const StageObject *diagnosticObject=nullptr;
bool diagnosticArticulated=false;
unsigned diagnosticFrame=0,diagnosticChanges=0;uint64_t diagnosticTime=0;
unsigned loggedSecond=0,treeReplacementDraws=0,treeWindDraws=0;

struct Part { std::vector<GfxVertex> vertices; PackTexture *texture=nullptr; };
struct Asset { std::map<const Mesh*,std::vector<Part>> fitted; int sourceWidth=0,sourceHeight=0; SceneryFormat::Entry entry; std::string failure; std::set<std::string> loggedFailures; bool attempted=false,ready=false; std::vector<Part> parts; std::vector<PackTexture*> frames; };
std::vector<Asset> assets;
bool loaded=false,identityContract=false;
bool treeLogged=false,spectatorLogged=false;
std::string root;
std::map<std::string,PackTexture> textures;
bool Read(const std::string &path,std::string &out) {
    SysFile *f=nullptr;
    if(Sys_HasSceneryDirectory()){
        // Paths already carry the configured relative pack prefix. Remove only
        // that exact prefix when reading from the independently selected root.
        std::string prefix=Sys_GetOptionText("video.scenery_pack","scenery-pack");if(!prefix.empty()&&prefix.back()!='/')prefix+='/';
        if(path.compare(0,prefix.size(),prefix)!=0)return false;
        std::string relative=path.substr(prefix.size());if(!SafePath(relative))return false;
        f=Sys_OpenSceneryFile(relative.c_str());
    }else f=Sys_OpenFile(path.c_str(),SYS_FILE_READ);
    if(!f)return false;
    DWORD n=Sys_GetFileSize(f);if(n>16*1024*1024){Sys_CloseFile(f);return false;}
    out.resize(n);bool ok=Sys_ReadFile(f,out.data(),n)==n;Sys_CloseFile(f);return ok;
}
std::string Dir(const std::string &s){auto at=s.find_last_of('/');return at==s.npos?"":s.substr(0,at+1);}
PackTexture *Load(const std::string &relative){
    auto cached=textures.find(relative);if(cached!=textures.end())return &cached->second;
    if(!SafePath(relative))return nullptr;
    std::string bytes,path=root+relative;if(!Read(path,bytes)||bytes.size()<18)return nullptr;
    // Decoder accepts uncompressed truecolour only. Validate dimensions/size first.
    const unsigned char *b=(const unsigned char*)bytes.data();unsigned w=b[12]|b[13]<<8,h=b[14]|b[15]<<8;
    if(b[1]!=0||b[2]!=2||b[16]!=32||w==0||h==0||w>4096||h>4096||bytes.size()<18u+b[0]+size_t(w)*h*4)return nullptr;
    if(b[4]||b[5]||b[6]||b[7]||b[8]||b[9]||b[10]||b[11]||b[17]!=8)return nullptr;
    GfxTexture *surface=Gfx_CreateTexture(w,h,GFX_FORMAT_BGRA8,1,0);if(!surface)return nullptr;
    GfxLockedRect lock={};if(!Gfx_LockTexture(surface,0,0,&lock)){Gfx_DestroyTexture(surface);return nullptr;}
    // TGA rows begin at bottom; renderer UV v=0 is top, as native TGA path.
    for(unsigned y=0;y<h;y++)memcpy((char*)lock.pixels+y*lock.pitch,b+18+b[0]+(h-1-y)*w*4,w*4);
    Gfx_UnlockTexture(surface,0,0);ownedTextures.push_back(surface);
    auto &t=textures[relative];t={surface,(int)w,(int)h};return &t;
}
void Init(){
    if(loaded)return;loaded=true;
    root=Sys_GetOptionText("video.scenery_pack","scenery-pack");if(!root.empty()&&root.back()!='/')root+='/';
    std::string text;if(!Read(root+"manifest.tsv",text))return;
    std::istringstream in(text);std::string line;
    while(std::getline(in,line)) { SceneryFormat::Entry e;if(ParseEntry(line,e)){
        bool duplicate=false;for(auto &a:assets)if(a.entry.archive==e.archive&&a.entry.source==e.source)duplicate=true;
        if(duplicate){Sys_Log("scenery pack: duplicate mapping rejected %s/%s",e.archive.c_str(),e.source.c_str());assets.clear();return;}
        if(assets.size()>=2048)return;Asset a;a.entry=e;assets.push_back(std::move(a));
    }}
    if(Read(root+"source-identities.tsv",text)) {
        identityContract=true;
        std::istringstream rows(text);
        while(std::getline(rows,line)) {auto p=Split(line,'\t');
            if(p.size()!=3||p[0].find('/')==p[0].npos||p[2].size()!=64||identities.size()>=2048)continue;
            if(p[2].find_first_not_of("0123456789abcdef")!=p[2].npos)continue;
            auto slash=p[0].find('/');identities.push_back({p[0].substr(0,slash),Stem(p[0].substr(slash+1)),p[2]});
        }
    }
    if(Read(root+"tree-wind-identities.tsv",text)){
        std::istringstream rows(text);while(std::getline(rows,line)){auto p=Split(line,'\t');
            if(p.size()!=3||p[2].size()!=64||p[2].find_first_not_of("0123456789abcdef")!=p[2].npos)continue;
            auto slash=p[0].find('/');if(slash!=p[0].npos)windIdentities[Stem(p[0].substr(slash+1))].insert(p[2]);
        }
    }
    Sys_Log("scenery pack: %u mappings, %u verified-byte identity routes",(unsigned)assets.size(),(unsigned)identities.size());
}
bool Reject(Asset &a,const char *reason){a.failure=reason;return false;}
void LogRejection(Asset &a){
    if(!a.loggedFailures.insert(a.failure).second)return;
    Sys_Log("options: scenery rejected %s/%s asset=%s: %s; keeping original",a.entry.archive.c_str(),
        a.entry.source.c_str(),a.entry.asset.c_str(),a.failure.c_str());
}
bool Prepare(Asset &a){
    if(a.attempted)return a.ready;a.attempted=true;
    if(a.entry.kind=="spectator"){
        int w=0,h=0;for(auto &path:a.entry.frames){PackTexture *t=Load(path);if(!t)return Reject(a,"spectator frame missing, invalid TGA or texture allocation failed");
            if(w&&(t->width!=w||t->height!=h))return Reject(a,"spectator frame dimensions disagree");w=t->width;h=t->height;a.frames.push_back(t);}
        return a.ready=!a.frames.empty();
    }
    std::string text;if(!Read(root+a.entry.asset,text))return Reject(a,"tree OBJ missing or unreadable");
    // Legacy cylindrical/repeat assets are deliberately rejected: they must
    // be regenerated with the source-canvas contract, not silently distorted.
    { std::istringstream header(text);std::string hash,version;
      if(!(header>>hash>>version>>a.sourceWidth>>a.sourceHeight)||hash!="#"||version!="opencmr2-source-canvas-1"||
         a.sourceWidth<1||a.sourceHeight<1||a.sourceWidth>4096||a.sourceHeight>4096)return Reject(a,"missing/invalid source-canvas-1 header; regenerate pack with current converter into a NEW folder"); }
    std::vector<Group> groups;if(!ParseOBJ(text,groups))return Reject(a,"tree OBJ geometry invalid");
    std::istringstream obj(text);std::string line,tag,mtl;
    while(std::getline(obj,line)){std::istringstream in(line);in>>tag;if(tag=="mtllib")in>>mtl;}
    if(!SafePath(mtl)||!Read(root+Dir(a.entry.asset)+mtl,text))return Reject(a,"tree material missing, unreadable or unsafe path");
    std::map<std::string,std::string> maps;std::string material;
    std::istringstream input(text);
    while(std::getline(input,line)){std::istringstream in(line);in>>tag;
        if(tag=="newmtl")in>>material;else if(tag=="map_Kd"){std::string p;in>>p;if(!SafePath(p))return Reject(a,"unsafe material texture path");maps[material]=Dir(a.entry.asset)+Dir(mtl)+p;}}
    for(auto &g:groups){auto it=maps.find(g.material);if(it==maps.end())return Reject(a,"tree material has no texture mapping");
        Part p;p.texture=Load(it->second);if(!p.texture)return Reject(a,"tree texture missing, invalid TGA or texture allocation failed");
        for(auto &v:g.vertices)p.vertices.push_back({v.x,v.y,v.z,v.nx,v.ny,v.nz,0xffffffff,0,v.u,v.v,0,0});
        a.parts.push_back(std::move(p));}
    return a.ready=true;
}
void State(const GfxMatrix &m){
    Gfx_PushState();Gfx_SetTransform(GFX_TRANSFORM_WORLD,&m);Gfx_SetLighting(FALSE);
    Gfx_SetDepthTest(TRUE);Gfx_SetDepthWrite(TRUE);Gfx_SetCullMode(GFX_CULL_NONE);
    Gfx_SetAlphaBlend(TRUE);Gfx_SetSrcBlend(GFX_BLEND_SRCALPHA);Gfx_SetDestBlend(GFX_BLEND_INVSRCALPHA);
    Gfx_SetAlphaTest(TRUE);Gfx_SetAlphaFunc(GFX_CMP_GREATER);Gfx_SetAlphaRef(127);
    Gfx_SetStageColorOp(0,GFX_TOP_MODULATE);Gfx_SetStageColorArg1(0,GFX_TA_TEXTURE);Gfx_SetStageColorArg2(0,GFX_TA_DIFFUSE);
    Gfx_SetStageAlphaOp(0,GFX_TOP_MODULATE);Gfx_SetStageAlphaArg1(0,GFX_TA_TEXTURE);Gfx_SetStageAlphaArg2(0,GFX_TA_DIFFUSE);
    Gfx_SetStageMinFilter(0,GFX_FILTER_POINT);Gfx_SetStageMagFilter(0,GFX_FILTER_POINT);Gfx_SetStageMipFilter(0,GFX_MIPFILTER_NONE);
    Gfx_SetStageTexCoordIndex(0,0);Gfx_SetStageTextureTransform(0,0);Gfx_SetStageAddress(0,GFX_ADDRESS_WRAP);
    Gfx_SetStageColorOp(1,GFX_TOP_DISABLE);Gfx_SetStageAlphaOp(1,GFX_TOP_DISABLE);
}
}
void SceneryPack_LogStatus(){
    if(!Sys_GetOption("video.scenery_overrides",1)){
        Sys_Log("options: scenery=original (overrides disabled)");return;
    }
    std::string folder=Sys_GetOptionText("video.scenery_pack","scenery-pack");
    if(!Sys_HasSceneryDirectory()&&!SceneryFormat::SafePath(folder)){
        Sys_Log("options: scenery=original (invalid relative pack root)");return;
    }
    std::string text;
    if(!Read(folder+"/manifest.tsv",text)){
        Sys_Log("options: scenery=original (manifest missing/unreadable)");return;
    }
    std::istringstream input(text);std::string line;unsigned count=0;
    while(std::getline(input,line)){SceneryFormat::Entry entry;if(ParseEntry(line,entry))count++;}
    Sys_Log("options: scenery manifest=%u parsed mappings; NOT active yet, resources checked lazily at stage draw",count);
}
void SceneryPack_RegisterSource(Texture *texture,const void *bytes,unsigned size){
    if(!texture)return;
    sourceIdentities.erase(texture); // allocator may reuse a released texture address
    if(!bytes||size<1||size>16*1024*1024||!Sys_GetOption("video.scenery_overrides",1))return;
    Init();std::string stem=Stem(texture->name);bool candidate=false;
    for(auto &a:assets)if(a.entry.source==stem){candidate=true;break;}
    if(windIdentities.count(stem))candidate=true;
    if(candidate)sourceIdentities[texture]=SceneryIdentity::SHA256(bytes,size);
}
void SceneryPack_RegisterArchive(GenericFile *file,const char *path){if(file&&path)archives[file]=Leaf(path);}
struct ReactCar{float x=0,z=0,speed=0;bool valid=false;uint64_t at=0;};static ReactCar reactCars[8];
void SceneryPack_Clear(){assets.clear();windIdentities.clear();windLogged.clear();treeOriginalLogged.clear();archives.clear();sourceIdentities.clear();identities.clear();unmatched.clear();for(auto t:ownedTextures)Gfx_DestroyTexture(t);ownedTextures.clear();textures.clear();loaded=false;identityContract=false;treeLogged=false;spectatorLogged=false;root.clear();animationTime=0;for(auto &c:reactCars)c=ReactCar();diagnosticObject=nullptr;diagnosticArticulated=false;diagnosticFrame=diagnosticChanges=loggedSecond=treeReplacementDraws=treeWindDraws=0;diagnosticTime=0;}
void SceneryPack_Advance(int step){if(step>0)animationTime+=(unsigned)step;}
void SceneryPack_SetReactionSource(int slot,int x,int z,int speed,bool valid){if(slot<0||slot>=8)return;reactCars[slot]={x/65536.f,z/65536.f,speed/65536.f,valid,animationTime};}
static bool DrawReplacement(StageObject *object,const float *billboard,unsigned textureCount,Texture **textureBuffer){
    if(!object||!object->pMesh||!billboard||Sys_GetOption("video.scenery_overrides",1)==0)return false;
    Mesh *mesh=object->pMesh;
    // Only replace single-source cutouts, never a composite stage mesh.
    if(!(mesh->flags&2)||!mesh->pTriangles||mesh->triangleCount<1||!mesh->pVertexData||mesh->field_0x10<1)return false;
    int texture=mesh->pTriangles[0].textures[0];if(texture<0||(unsigned)texture>=textureCount)return false;
    for(int i=1;i<mesh->triangleCount;i++)if(mesh->pTriangles[i].textures[0]!=texture)return false;
    Texture *source=textureBuffer[texture];if(!source)return false;
    auto archive=archives.find((GenericFile*)source->pArchive);
    Init();Asset *asset=nullptr;std::string stem=Stem(source->name);
    auto identity=sourceIdentities.find(source);
    // Source-index archives are preparation containers, not necessarily the
    // runtime stage archive. Loose textures have no pArchive at all. Route
    // these only by exact original DDS bytes, never by an ambiguous filename.
    if(identity!=sourceIdentities.end())for(auto &id:identities)if(id.source==stem&&id.hash==identity->second)
        for(auto &a:assets)if(a.entry.archive==id.archive&&a.entry.source==stem){asset=&a;break;}
    // Legacy packs retain their strict archive+stem contract, but new packs
    // with identity records never bypass a failing byte check.
    if(!identityContract&&archive!=archives.end())
        for(auto &a:assets)if(a.entry.archive==archive->second&&a.entry.source==stem){asset=&a;break;}
    if(!asset){
        for(auto &a:assets)if(a.entry.source==stem&&unmatched.insert(stem).second)
            Sys_Log("options: scenery source unmatched %s runtime archive=%s; source bytes unverified or pack lacks matching identity; keeping original",stem.c_str(),archive==archives.end()?"loose/unregistered":archive->second.c_str());
        return false;
    }
    if(asset->entry.kind=="tree"&&Sys_GetOption("video.tree_overrides",1)==0)return false;
    if(asset->entry.kind=="spectator"&&Sys_GetOption("video.spectator_overrides",1)==0)return false;
    if(!Prepare(*asset)){
        LogRejection(*asset);
        return false;
    }
    auto v=(MeshVertexF*)mesh->pVertexData;
    float bottom=v[0].y,top=v[0].y;
    for(int i=1;i<mesh->field_0x10;i++){bottom=std::min(bottom,v[i].y);top=std::max(top,v[i].y);}
    float height=top-bottom;if(!std::isfinite(height)||height<=0)return false;
    if(asset->entry.kind=="spectator"){
        // The stage-effects update owns time. Drawing and reflection passes never
        // advance it. Stable placement phase does not consume physics RNG.
        unsigned phase=((unsigned)object->position.x*1664525u)^(unsigned)object->position.z;
        unsigned frame=Frame(animationTime,asset->entry.ticks,(unsigned)asset->frames.size(),phase);
        // Every identity-verified supported card moves, including neighbours on
        // multi-person atlases. Authored limb poses remain in addition to sway.
        bool selected=true; // Requested full supported placement coverage.
        if(!selected)frame=0;
        // Any nearby rally car excites the crowd. A car stops counting after ~2s of authored
        // time without a live-race refresh (menus, pauses, replays that do not update).
        float excitement=0;
        if(Sys_GetOption("video.reactive_spectators",1)!=0)for(const auto &c:reactCars)if(c.valid&&animationTime-c.at<65536ull*50)
            excitement=std::max(excitement,SceneryMotion::Excitement(object->position.x/65536.f-c.x,object->position.z/65536.f-c.z,c.speed));
        PackTexture *t=asset->frames[frame];std::vector<GfxVertex> vertices;
        for(int i=0;i<mesh->triangleCount;i++)for(int j=0;j<3;j++){
            unsigned ix=mesh->pTriangles[i].vertexIndex[j];if(ix>=(unsigned)mesh->field_0x10)return false;
            auto &p=v[ix];vertices.push_back({p.x+SceneryMotion::ReactiveOffset(p.y,bottom,height,animationTime,phase,excitement),p.y,p.z,p.nx,p.ny,p.nz,p.diffuseColour,p.specularColour,p.textureCoordinates[0][0],p.textureCoordinates[0][1],0,0});}
        if(!spectatorLogged){Sys_Log("options: spectator sidecar active (first successful override draw)");spectatorLogged=true;}
        State(*(const GfxMatrix*)billboard);Gfx_SetTexture(0,t->surface);Gfx_DrawPrimitive(GFX_TRIANGLELIST,vertices.data(),vertices.size());Gfx_PopState();
        if(selected&&(!diagnosticObject||(asset->frames.size()==4&&!diagnosticArticulated))){diagnosticObject=object;diagnosticFrame=frame;diagnosticChanges=0;diagnosticArticulated=asset->frames.size()==4;}
        if(diagnosticObject==object){
            if(diagnosticFrame!=frame)++diagnosticChanges;
            diagnosticFrame=frame;diagnosticTime=animationTime;
            unsigned second=(unsigned)(animationTime/(25ull*65536));
            if(second!=loggedSecond&&second<=5){
                Sys_Log("options: spectator animation tick=%llu frame=%u/%u bound-frame-changes=%u texture=%s",(unsigned long long)(animationTime/65536),frame,(unsigned)asset->frames.size(),diagnosticChanges,asset->entry.frames[frame].c_str());
                loggedSecond=second;
            }
        }
        return true;
    }
    if(source->width!=asset->sourceWidth||source->height!=asset->sourceHeight){
        Reject(*asset,"source texture dimensions differ from tree header");LogRejection(*asset);return false;
    }
    auto fitted=asset->fitted.find(mesh);
    if(fitted==asset->fitted.end()){
        ScenerySourceFit::Fit fit;
        if(!ScenerySourceFit::Build(v,mesh->field_0x10,fit)){
            asset->fitted.emplace(mesh,std::vector<Part>{});Reject(*asset,"source card is cropped, nonplanar or invalid for full-canvas fit");LogRejection(*asset);return false;
        }
        std::vector<Part> parts=asset->parts;
        for(auto &part:parts)for(auto &q:part.vertices){float xyz[3];
            float u=.5f+q.x*asset->sourceHeight/asset->sourceWidth;
            float sv=1.f-q.y;
            fit.Map(u,sv,q.z,xyz);
            if(!std::isfinite(xyz[0])||!std::isfinite(xyz[1])||!std::isfinite(xyz[2])){
                asset->fitted.emplace(mesh,std::vector<Part>{});Reject(*asset,"source-fit produced nonfinite geometry");LogRejection(*asset);return false;
            }
            q.x=xyz[0];q.y=xyz[1];q.z=xyz[2];
        }
        fitted=asset->fitted.emplace(mesh,std::move(parts)).first;
    }
    if(fitted->second.empty())return false;
    ++treeReplacementDraws;
    // Apply placement basis exactly once. Source-fit positions are already in
    // original mesh units, not normalized geometry multiplied by height again.
    const FixMatrix &f=object->matrix;
    GfxMatrix m={f.right.x/65536.f,f.right.y/65536.f,f.right.z/65536.f,0,
        f.up.x/65536.f,f.up.y/65536.f,f.up.z/65536.f,0,
        f.forward.x/65536.f,f.forward.y/65536.f,f.forward.z/65536.f,0,
        f.position.x/65536.f,f.position.y/65536.f,f.position.z/65536.f,1};
    if(!treeLogged){Sys_Log("options: tree sidecar active (source-fitted successful override draw)");treeLogged=true;}
    State(m);
    unsigned phase=((unsigned)object->position.x*1664525u)^(unsigned)object->position.z;
    for(auto &p:fitted->second){
        // Bend fitted replacement, too. The accepted source-fit tree shape is
        // unchanged at rest and original/fitted buffers are never rewritten.
        if(Sys_GetOption("video.tree_wind",0)) {
            auto moved=p.vertices;
            for(auto &q:moved)q.x+=SceneryMotion::Offset(q.y,bottom,height,animationTime,phase,true);
            Gfx_SetTexture(0,p.texture->surface);Gfx_DrawPrimitive(GFX_TRIANGLELIST,moved.data(),moved.size());
        } else {Gfx_SetTexture(0,p.texture->surface);Gfx_DrawPrimitive(GFX_TRIANGLELIST,p.vertices.data(),p.vertices.size());}
    }
    Gfx_PopState();return true;
}

bool SceneryPack_GetAnimationStatus(unsigned long long *tick,unsigned *frame,unsigned *changes){
    if(!diagnosticObject)return false;
    if(tick)*tick=(unsigned long long)(diagnosticTime/65536);
    if(frame)*frame=diagnosticFrame;
    if(changes)*changes=diagnosticChanges;
    return true;
}

bool SceneryPack_Draw(StageObject *object,const float *billboard){
    if(DrawReplacement(object,billboard,CGraphics::m_textureCount,CGraphics::GetTextureBuffer()))return true;
    if(!object||!object->pMesh||!billboard||!Sys_GetOption("video.scenery_overrides",1))return false;
    auto mesh=object->pMesh;
    if(!(mesh->flags&2)||!mesh->pTriangles||mesh->triangleCount<1||mesh->triangleCount>64||!mesh->pVertexData||mesh->field_0x10<3||mesh->field_0x10>64)return false;
    int index=mesh->pTriangles[0].textures[0];if(index<0||(unsigned)index>=CGraphics::m_textureCount)return false;
    for(int i=1;i<mesh->triangleCount;i++)if(mesh->pTriangles[i].textures[0]!=index)return false;
    auto source=CGraphics::GetTextureBuffer()[index];if(!source||!source->pSurface)return false;
    Init();auto stem=Stem(source->name);auto wind=windIdentities.find(stem);
    if(wind==windIdentities.end()){
        if(stem.compare(0,4,"tre_")==0&&treeOriginalLogged.insert(stem).second)Sys_Log("options: tree original %s: not covered by verified replacement/wind routes",stem.c_str());
        return false;
    }
    auto id=sourceIdentities.find(source);
    if(!Sys_GetOption("video.tree_wind",0)||id==sourceIdentities.end()||!wind->second.count(id->second))return false;
    auto vertices=(MeshVertexF*)mesh->pVertexData;float bottom=vertices[0].y,top=bottom;
    for(int i=0;i<mesh->field_0x10;i++){bottom=std::min(bottom,vertices[i].y);top=std::max(top,vertices[i].y);}
    float height=top-bottom;if(!std::isfinite(height)||height<=0)return false;
    float phase=((unsigned)object->position.x^(unsigned)object->position.z)%6283*.001f;

    std::vector<GfxVertex> out;
    for(int i=0;i<mesh->triangleCount;i++)for(int j=0;j<3;j++){
        unsigned ix=mesh->pTriangles[i].vertexIndex[j];if(ix>=(unsigned)mesh->field_0x10)return false;
        auto &v=vertices[ix];float motion=SceneryMotion::Offset(v.y,bottom,height,animationTime,(unsigned)(phase*1000),true);
        out.push_back({v.x+motion,v.y,v.z,v.nx,v.ny,v.nz,v.diffuseColour,v.specularColour,v.textureCoordinates[0][0],v.textureCoordinates[0][1],0,0});
    }
    State(*(const GfxMatrix*)billboard);Gfx_SetTexture(0,source->pSurface);Gfx_DrawPrimitive(GFX_TRIANGLELIST,out.data(),out.size());Gfx_PopState();
    ++treeWindDraws;
    if(windLogged.insert(stem).second)Sys_Log("options: tree wind active %s (original2Dcard, fixed base; NOT3Dreplacement)",stem.c_str());
    return true;
}

void SceneryPack_GetTreeStatus(unsigned *replacements,unsigned *wind,unsigned *originalSources){
 if(replacements)*replacements=treeReplacementDraws;
 if(wind)*wind=treeWindDraws;
 if(originalSources)*originalSources=(unsigned)treeOriginalLogged.size();
}
