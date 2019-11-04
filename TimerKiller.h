#ifndef DISTRIBUTED_ALGORITHMS_TIMERKILLER_H
#define DISTRIBUTED_ALGORITHMS_TIMERKILLER_H

#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <future>
#include <chrono>
#include <iostream>
#include <vector>

using namespace std;

// This class is freely inspired from the implementation found in
// http://coliru.stacked-crooked.com/a/66f8b2ee5f9d55e8

struct timer_killer {
    // returns false if killed:
    template<class R, class P>
    bool wait_for( chrono::duration<R,P> const& time ) {
        unique_lock<std::mutex> lock(m);
        return !cv.wait_for(lock, time, [&]{return terminate;});
    }
    void kill() {
        unique_lock<std::mutex> lock(m);
        terminate=true;
        cv.notify_all();
    }
private:
    condition_variable cv;
    mutex m;
    bool terminate = false;
};

/*
 * This data structure contains timer_killers for each possible sender,
 * ordered by process_number first, and then by (message) sequence_number
 */
extern timer_killer timer_killer_by_process_message[1000][1000];
// vector<timer_killer> myv;


#endif //DISTRIBUTED_ALGORITHMS_TIMERKILLER_H
