#ifndef OPENCMR2_CROWD_MESH_H
#define OPENCMR2_CROWD_MESH_H
#include <vector>
// Splits a triangle into 16 (two midpoint splits). Position, normal and texture coordinates are interpolated, other
// fields come from the first corner, so the silhouette and the texture mapping do not change. V needs x y z nx ny nz u0 v0.
namespace CrowdMesh {
template<class V> V Mid(const V &a,const V &b){V m=a;m.x=(a.x+b.x)*.5f;m.y=(a.y+b.y)*.5f;m.z=(a.z+b.z)*.5f;m.nx=(a.nx+b.nx)*.5f;m.ny=(a.ny+b.ny)*.5f;m.nz=(a.nz+b.nz)*.5f;m.u0=(a.u0+b.u0)*.5f;m.v0=(a.v0+b.v0)*.5f;return m;}
template<class V> void Split4(const V &a,const V &b,const V &c,V *out){V ab=Mid(a,b),bc=Mid(b,c),ca=Mid(c,a);const V q[12]={a,ab,ca,ab,b,bc,ca,bc,c,ab,bc,ca};for(int k=0;k<12;k++)out[k]=q[k];}
template<class V> void Tessellate16(const V &a,const V &b,const V &c,std::vector<V> &out){
 V first[12];Split4(a,b,c,first);
 for(int k=0;k<4;k++){V second[12];Split4(first[k*3],first[k*3+1],first[k*3+2],second);for(int m=0;m<12;m++)out.push_back(second[m]);}
}
// Per-mesh template, built once: tessellated vertices at rest plus the source bounds the sway needs. valid() says
// whether it still describes a mesh with these dimensions (a mesh pointer can be reused by another mesh after a stage change).
template<class V> struct Template {
 std::vector<V> verts;int triangles=0,sourceVertices=0;float bottom=0,height=0,left=0,span=0;
 bool Matches(int tris,int vcount,float b,float h,float l,float s) const {return !verts.empty()&&tris==triangles&&vcount==sourceVertices&&b==bottom&&h==height&&l==left&&s==span;}
};
}
#endif
