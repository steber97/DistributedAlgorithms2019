#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>

struct timer_killer {
    // returns false if killed:
    template<class R, class P>
    bool wait_for( std::chrono::duration<R,P> const& time ) {
        std::unique_lock<std::mutex> lock(m);
        return !cv.wait_for(lock, time, [&]{return terminate;});
    }
    void kill() {
        std::unique_lock<std::mutex> lock(m);
        terminate=true;
        cv.notify_all();
    }
private:
    std::condition_variable cv;
    std::mutex m;
    bool terminate = false;
};

timer_killer bob;

void wait(){
    bool a = true;
    while(a){
        std::cout << "another loop" << std::endl;
        a = bob.wait_for(std::chrono::milliseconds(1000));
    }
}

int main() {

    std::thread t(wait);
    std::cout << "killing threads\n";

    int a;
    std::cin >> a;

    bob.kill();

    t.join();
    std::cout << "done\n";
    // your code goes here
    return 0;
}