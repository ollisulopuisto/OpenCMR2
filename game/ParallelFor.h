// OpenCMR2: a small fixed worker pool for visual-only CPU preparation (no graphics, no game state).
// ParallelFor(n, fn) calls fn(i) once for every i in [0,n) and returns when all are done. fn must only
// read shared data and write to slot i. With 1 thread, or n < 2, everything runs on the calling thread
// in index order, so the serial path is the fallback and the reference.
#ifndef OPENCMR2_PARALLELFOR_H
#define OPENCMR2_PARALLELFOR_H
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
namespace ParallelWork {
class Pool {
public:
    // threads: total including the caller. 0 = hardware_concurrency (capped at 8), 1 = serial.
    explicit Pool(unsigned threads=0){
        if(threads==0){threads=std::thread::hardware_concurrency();if(threads>8)threads=8;}
        if(threads<1)threads=1;
        total=threads;
        try{for(unsigned i=1;i<threads;i++)workers.emplace_back([this]{Loop();});}
        catch(...){/* could not start a thread: run with the ones that started */ total=(unsigned)workers.size()+1;}
    }
    ~Pool(){
        {std::lock_guard<std::mutex> l(m);quit=true;++generation;}
        cv.notify_all();
        for(auto &t:workers)t.join();
    }
    Pool(const Pool&)=delete;Pool &operator=(const Pool&)=delete;
    unsigned Threads()const{return total;}
    void Run(size_t n,const std::function<void(size_t)> &fn){
        if(total<2||n<2){for(size_t i=0;i<n;i++)fn(i);return;}
        std::lock_guard<std::mutex> serialize(runMutex);
        {std::lock_guard<std::mutex> l(m);job=&fn;count=n;next.store(0);active=(unsigned)workers.size();++generation;}
        cv.notify_all();
        Drain();
        std::unique_lock<std::mutex> l(m);
        done.wait(l,[this]{return active==0;});
        job=nullptr;
    }
private:
    void Drain(){
        // Small chunks keep the load even without making the shared counter the bottleneck.
        const size_t chunk=1;
        for(;;){size_t i=next.fetch_add(chunk);if(i>=count)break;(*job)(i);}
    }
    void Loop(){
        unsigned seen=0;
        for(;;){
            {std::unique_lock<std::mutex> l(m);cv.wait(l,[&]{return generation!=seen;});seen=generation;if(quit)return;}
            Drain();
            {std::lock_guard<std::mutex> l(m);if(--active==0)done.notify_one();}
        }
    }
    unsigned total=1,generation=0,active=0;bool quit=false;
    size_t count=0;std::atomic<size_t> next{0};const std::function<void(size_t)> *job=nullptr;
    std::mutex m,runMutex;std::condition_variable cv,done;std::vector<std::thread> workers;
};
}
#endif
