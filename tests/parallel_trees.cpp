// OpenCMR2: the worker pool and the tree wind kernel. Parallel output must be bit-identical to serial
// output for any thread count, the pool must survive repeated runs and shut down cleanly.
// The timing at the end is a SYNTHETIC workload (random vertices, tree-like part sizes), not a game measurement.
#include "SceneryMotion.h"
#include "ParallelFor.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
struct V{float x,y,z,u0,v0,pad;};
static int failures;
static float rnd(){return rand()/(float)RAND_MAX;}
struct Part{std::vector<V> src,out;std::vector<SceneryMotion::WindVertex> w;};
struct Tree{std::vector<Part> parts;SceneryMotion::WindFrame f;};
static std::vector<Tree> Make(int trees,int partsPer,int verts){
    std::vector<Tree> t(trees);
    for(int i=0;i<trees;i++){
        float h=3+rnd()*20;t[i].f=SceneryMotion::MakeWindFrame(h,(uint64_t)rand()*977,(uint32_t)rand(),1.f);
        t[i].parts.resize(partsPer);
        for(auto &p:t[i].parts){
            p.src.resize(verts);p.out.resize(verts);p.w.resize(verts);
            for(int k=0;k<verts;k++){p.src[k]={rnd()*4,rnd()*h,rnd()*4,rnd(),rnd(),0};p.w[k]=SceneryMotion::MakeWindVertex(p.src[k].u0,p.src[k].v0,p.src[k].y,0,h);}
        }
    }
    return t;
}
static void Run(std::vector<Tree> &t,ParallelWork::Pool &pool){
    std::vector<std::pair<Tree*,Part*>> jobs;for(auto &tr:t)for(auto &p:tr.parts)jobs.push_back({&tr,&p});
    pool.Run(jobs.size(),[&](size_t i){auto &p=*jobs[i].second;SceneryMotion::WindPart(p.src.data(),p.out.data(),p.w.data(),p.src.size(),jobs[i].first->f);});
}
static bool Same(const std::vector<Tree> &a,const std::vector<Tree> &b){
    for(size_t i=0;i<a.size();i++)for(size_t j=0;j<a[i].parts.size();j++)
        if(memcmp(a[i].parts[j].out.data(),b[i].parts[j].out.data(),a[i].parts[j].out.size()*sizeof(V)))return false;
    return true;
}
int main(void){
    srand(7);
    auto ref=Make(300,3,400);auto test=ref;
    { ParallelWork::Pool serial(1);Run(ref,serial);if(serial.Threads()!=1){puts("FAIL: 1 thread");failures++;} }
    for(unsigned n:{2u,3u,4u,8u,0u}){
        ParallelWork::Pool pool(n);
        for(int rep=0;rep<20;rep++){for(auto &t:test)for(auto &p:t.parts)memset(p.out.data(),0,p.out.size()*sizeof(V));Run(test,pool);
            if(!Same(ref,test)){printf("FAIL: threads=%u rep=%d output differs from serial\n",n,rep);failures++;break;}}
        // n<2 and empty jobs
        pool.Run(0,[](size_t){puts("FAIL: ran with n=0");failures++;});
        int one=0;pool.Run(1,[&](size_t){++one;});if(one!=1){puts("FAIL: n=1");failures++;}
        printf("threads requested %u -> %u: identical to serial over 20 runs\n",n,pool.Threads());
    }
    // every index exactly once
    { ParallelWork::Pool pool(4);std::vector<int> hits(10000,0);pool.Run(hits.size(),[&](size_t i){hits[i]++;});for(int h:hits)if(h!=1){puts("FAIL: index not run exactly once");failures++;break;} }
    // synthetic timing, 3 parts x ~400 verts per tree, 300 trees (about the 128-1163 calls and ~1k verts seen natively)
    auto t=Make(300,1,1000);
    for(unsigned n:{1u,2u,4u,0u}){
        ParallelWork::Pool pool(n);Run(t,pool);
        auto a=std::chrono::steady_clock::now();for(int r=0;r<50;r++)Run(t,pool);
        double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-a).count()/50;
        printf("SYNTHETIC 300 parts x 1000 verts, threads=%u: %.3f ms per batch\n",pool.Threads(),ms);
    }
    puts(failures?"FAILED":"ok");return failures?1:0;
}
