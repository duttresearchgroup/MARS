#ifndef __semaphore_h
#define __semaphore_h

#include <condition_variable>
#include <mutex>

class Semaphore {
private:
    int count;
    std::mutex mtx;
    std::condition_variable cv;

public:
    explicit Semaphore(int count_ = 1) :count(count_){ }

    void notify()
    {
        std::unique_lock<std::mutex> lck(mtx);
        ++count;
        cv.notify_one();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lck(mtx);

        while(count <= 0){
            cv.wait(lck);
        }
        --count;
    }

    bool tryWait()
    {
        std::unique_lock<std::mutex> lck(mtx);
        if(count > 0) {
            --count;
            return true;
        }
        else {
            return false;
        }
    }

    int getCount()
    {
        std::unique_lock<std::mutex> lck(mtx);
        return count;
    }
};


#endif
