// OpenCMR2: queue of draw jobs whose preparation runs on worker threads but whose recording stays in queue order on the calling
// thread. Compute(fn) runs fn(job,out) for every queued job on the pool (fn must read only the job and shared immutable data and write
// only out). Record(fn) then calls fn(job,out) in the order the jobs were pushed and empties the queue. Flush = Compute + Record.
// The caller owns the rule that anything else that records, or changes data a queued job reads, calls Flush first.
#ifndef OPENCMR2_ORDEREDBATCH_H
#define OPENCMR2_ORDEREDBATCH_H
#include "ParallelFor.h"
#include <vector>
template<class Job,class Out>
class OrderedBatch{
public:
    void Push(const Job &j){jobs.push_back(j);computed=false;}
    bool Empty()const{return jobs.empty();}
    size_t Size()const{return jobs.size();}
    void Clear(){jobs.clear();computed=false;}
    template<class Compute> void ComputeAll(ParallelWork::Pool &pool,Compute fn){
        if(outs.size()<jobs.size())outs.resize(jobs.size());
        pool.Run(jobs.size(),[&](size_t i){fn(jobs[i],outs[i]);});
        computed=true;
    }
    template<class Record> void RecordAll(Record fn){
        for(size_t i=0;i<jobs.size();i++)fn(jobs[i],outs[i]);
        Clear();
    }
    bool Computed()const{return computed;}
private:
    std::vector<Job> jobs;std::vector<Out> outs;bool computed=false;
};
#endif
