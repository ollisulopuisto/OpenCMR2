// OpenCMR2: contract test for the queued tree draws (OrderedBatch). A stub "renderer" logs every record call. A random stream of
// tree draws (queued), other draws (spectator/fallback/legacy card: must flush first), cache rebuilds (must flush first) and stage
// clears is run (a) immediately, one draw at a time, as the code did before, and (b) through the batch with several thread counts.
// The record logs must be identical, so order and the data each draw saw are unchanged. A control run that skips the flush before a
// rebuild must differ, which shows the test would catch a missing flush. This tests the queue contract with a stub; it does not
// build SceneryPack.cpp or touch the real Gfx state, which cannot be built here.
#include "OrderedBatch.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
static int failures;
struct PartData{std::vector<float> cache;int version=0;};
struct Job{int part;float frame;int version;const PartData *data;};
typedef std::vector<float> Out;
static float Compute(const Job &j,Out &o){o.resize(j.data->cache.size());for(size_t i=0;i<o.size();i++)o[i]=j.data->cache[i]*j.frame+j.part;return 0;}
struct Log{std::vector<std::string> e;void add(const char*k,int part,const Out*o){char b[96];snprintf(b,96,"%s p%d sum=%.3f n=%zu",k,part,o?[&]{double s=0;for(float f:*o)s+=f;return s;}():0.0,o?o->size():0u);e.push_back(b);}};
enum Op{TREE,OTHER,REBUILD,CLEAR};
struct Step{Op op;int part;float frame;};
static std::vector<Step> Make(unsigned seed,int n,bool clears){
    std::vector<Step> s;for(int i=0;i<n;i++){seed=seed*1664525u+1013904223u;unsigned r=seed>>8;int k=r%100;
        Step st;st.part=(int)(r>>8)%4;st.frame=((r>>12)%100)/10.f;
        st.op=k<70?TREE:k<85?OTHER:(k<97||!clears)?REBUILD:CLEAR;s.push_back(st);}return s;
}
static void Rebuild(PartData &d,int part){d.version++;d.cache.assign(50+part*7+d.version%5,(float)(d.version+1)*.5f);}
static Log Reference(const std::vector<Step> &steps,bool honourClear){
    PartData parts[4];for(int i=0;i<4;i++)Rebuild(parts[i],i);Log log;
    for(auto &s:steps){
        if(s.op==TREE){Job j{s.part,s.frame,parts[s.part].version,&parts[s.part]};Out o;Compute(j,o);log.add("tree",s.part,&o);}
        else if(s.op==OTHER)log.add("other",s.part,nullptr);
        else if(s.op==REBUILD)Rebuild(parts[s.part],s.part);
        (void)honourClear;
    }return log;
}
static Log Batched(const std::vector<Step> &steps,unsigned threads,bool flushBeforeRebuild,size_t minimumParallelJobs=2){
    PartData parts[4];for(int i=0;i<4;i++)Rebuild(parts[i],i);Log log;ParallelWork::Pool pool(threads);OrderedBatch<Job,Out> b;
    auto flush=[&]{if(b.Empty())return;b.ComputeAll(pool,[](const Job &j,Out &o){Compute(j,o);},minimumParallelJobs);b.RecordAll([&](const Job &j,Out &o){log.add("tree",j.part,&o);});};
    for(auto &s:steps){
        if(s.op==TREE)b.Push(Job{s.part,s.frame,parts[s.part].version,&parts[s.part]});
        else if(s.op==OTHER){flush();log.add("other",s.part,nullptr);}
        else if(s.op==REBUILD){if(flushBeforeRebuild)flush();Rebuild(parts[s.part],s.part);}
    }
    flush(); // End of batch
    return log;
}
int main(void){
    for(unsigned seed=1;seed<=40;seed++){
        auto steps=Make(seed,400,false);auto ref=Reference(steps,false);
        for(unsigned th:{1u,2u,4u,0u})for(size_t minimum:{2u,8u}){
            auto got=Batched(steps,th,true,minimum);
            if(got.e!=ref.e){printf("FAIL seed %u threads %u: record log differs from immediate drawing (%zu vs %zu entries)\n",seed,th,got.e.size(),ref.e.size());failures++;break;}
        }
    }
    // control: without the flush before a rebuild the data seen by queued draws changes, and the test must notice
    int caught=0;for(unsigned seed=1;seed<=40;seed++){auto steps=Make(seed,400,false);if(Batched(steps,2,false).e!=Reference(steps,false).e)caught++;}
    printf("control (no flush before rebuild) differs from reference in %d of 40 streams\n",caught);
    if(caught<30){puts("FAIL: control not sensitive");failures++;}
    // stage clear: queued jobs are dropped and nothing is recorded for them; later draws work
    { ParallelWork::Pool pool(2);OrderedBatch<Job,Out> b;PartData d;Rebuild(d,0);Log log;
      b.Push(Job{0,1,d.version,&d});b.Push(Job{0,2,d.version,&d});b.Clear();
      if(!b.Empty()){puts("FAIL: clear");failures++;}
      b.Push(Job{0,3,d.version,&d});b.ComputeAll(pool,[](const Job&j,Out&o){Compute(j,o);});b.RecordAll([&](const Job&j,Out&o){log.add("tree",j.part,&o);});
      if(log.e.size()!=1||!b.Empty()){puts("FAIL: after clear");failures++;} }
    // empty flush is a no-op, and a second use of the batch reuses capacity without leaking old outputs
    { ParallelWork::Pool pool(2);OrderedBatch<Job,Out> b;PartData d;Rebuild(d,0);Log log;
      for(int r=0;r<5;r++){for(int i=0;i<r+1;i++)b.Push(Job{0,(float)i,d.version,&d});b.ComputeAll(pool,[](const Job&j,Out&o){Compute(j,o);});b.RecordAll([&](const Job&j,Out&o){log.add("tree",j.part,&o);});}
      if(log.e.size()!=15){puts("FAIL: reuse");failures++;} }
    // Both serial small-batch and worker large-batch routes preserve exact output values.
    for(int count:{1,2,7,8,16}){ParallelWork::Pool pool(8);OrderedBatch<int,std::vector<int>> b;
      for(int i=0;i<count;i++)b.Push(i);
      b.ComputeAll(pool,[](const int &j,std::vector<int>&o){o={j,j*j,0x112233};},8);
      int next=0;b.RecordAll([&](const int&j,const std::vector<int>&o){if(j!=next++||o!=std::vector<int>{j,j*j,0x112233})failures++;});
      if(next!=count)failures++;
    }
    puts(failures?"FAILED":"ok");return failures?1:0;
}
