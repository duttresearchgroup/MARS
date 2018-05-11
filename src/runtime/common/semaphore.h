/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef __arm_rt_sempahore_h
#define __arm_rt_sempahore_h

#include <condition_variable>
#include <mutex>

class Semaphore {
private:
    int count;
    std::mutex mtx;
    std::condition_variable cv;

public:
    explicit Semaphore(int count_ = 0) :count(count_){ }

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
