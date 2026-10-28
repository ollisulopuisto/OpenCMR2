#ifndef OPENCMR2_SCENERY_PACK_FORMAT_H
#define OPENCMR2_SCENERY_PACK_FORMAT_H
#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <cstdint>
namespace SceneryFormat {
inline std::vector<std::string> Split(const std::string &s, char c) {
    std::vector<std::string> out; std::istringstream in(s); std::string p;
    while(std::getline(in,p,c)) out.push_back(p); return out;
}
inline bool SafePath(const std::string &s) {
    if(s.empty() || s[0]=='/' || s.find('\\')!=s.npos || s.find(':')!=s.npos) return false;
    for(auto &p:Split(s,'/')) if(p.empty() || p=="." || p=="..") return false;
    return s.back()!='/';
}
inline std::string Leaf(std::string s) {
    size_t at=s.find_last_of("/\\"); if(at!=s.npos) s=s.substr(at+1);
    for(char &c:s) if(c>='A'&&c<='Z') c+=32; return s;
}
inline std::string Stem(std::string s) {
    s=Leaf(s); auto at=s.find_last_of('.'); return at==s.npos?s:s.substr(0,at);
}
inline int Step(float rate) { return std::isfinite(rate)&&rate>0&&rate<10000 ? int(65536.0*25.0/rate+0.5):0; }
inline unsigned Frame(uint64_t time,unsigned ticks,unsigned count,unsigned phase) {
    return ticks&&count ? unsigned((time/(65536ull*ticks)+phase)%count):0;
}
struct Entry { std::string kind,archive,source,asset; std::vector<std::string> frames; unsigned ticks=5; };
inline bool ParseEntry(std::string line, Entry &out) {
    if(!line.empty()&&line.back()=='\r') line.pop_back();
    auto p=Split(line,'\t'); if(p.size()!=6 || (p[0]!="tree"&&p[0]!="spectator")) return false;
    if(!SafePath(p[3])||p[1]!=Leaf(p[1])||p[1].empty()||p[2].empty()) return false;
    Entry e; e.kind=p[0];e.archive=p[1];e.source=Stem(p[2]);e.asset=p[3];
    char *end=nullptr; unsigned long t=std::strtoul(p[5].c_str(),&end,10);
    if(!end||*end||p[5].empty()||t<1||t>600) return false; e.ticks=(unsigned)t;
    if(e.kind=="spectator") { e.frames=Split(p[4],','); if(e.frames.empty()||e.frames.size()>64) return false;
        for(auto &f:e.frames) if(!SafePath(f)) return false; }
    out=e; return true;
}
struct Vertex { float x,y,z,nx,ny,nz,u,v; };
struct Group { std::string material; std::vector<Vertex> vertices; };
// Restricted, bounded OBJ subset. Reject malformed faces rather than partly drawing.
inline bool ParseOBJ(const std::string &text,std::vector<Group> &groups) {
    struct P { float x,y,z; }; std::vector<P> pos,norm,uv; std::vector<Group> result;
    std::istringstream input(text); std::string line,tag,name;
    while(std::getline(input,line)) { std::istringstream in(line); in>>tag;
        if(tag=="v"||tag=="vn"||tag=="vt") { P p={}; if(!(in>>p.x>>p.y)) return false;
            if(tag!="vt"&&!(in>>p.z)) return false;
            if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)) return false;
            auto &v=tag=="v"?pos:tag=="vn"?norm:uv; if(v.size()>=65536) return false;v.push_back(p);
        } else if(tag=="usemtl") { if(!(in>>name)||name.empty()) return false;result.push_back({name,{}}); }
        else if(tag=="f") { if(result.empty()) return false; std::string token;
            for(int j=0;j<3;j++){if(!(in>>token))return false;auto ix=Split(token,'/');if(ix.size()!=3)return false;
                unsigned long ids[3];for(int k=0;k<3;k++){char *end;ids[k]=strtoul(ix[k].c_str(),&end,10);if(*end||ix[k].empty()||ids[k]==0)return false;}
                if(ids[0]>pos.size()||ids[1]>uv.size()||ids[2]>norm.size())return false;
                P p=pos[ids[0]-1],t=uv[ids[1]-1],n=norm[ids[2]-1];
                auto &g=result.back();if(g.vertices.size()>=196608)return false;
                g.vertices.push_back({p.x,p.y,p.z,n.x,n.y,n.z,t.x,1-t.y});
            } if(in>>token && token[0]!='#')return false;
        }
    }
    if(result.empty())return false;for(auto &g:result)if(g.vertices.empty())return false;
    groups=std::move(result);return true;
}
}
#endif
