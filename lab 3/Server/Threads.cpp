#include <iterator>
#include <algorithm>

#include "Threads.hpp"

using namespace std;


Threads::Threads(size_t threadCount): terminate(false)
{
    if (threadCount==0)
        threadCount = thread::hardware_concurrency();
    threads.reserve(threadCount);
    generate_n(back_inserter(threads), threadCount, [this]() { return thread{ threadTask, this }; });
}

Threads::~Threads()
{
    clear();
    terminate = true;
    jobsAvailable.notify_all();

    for (auto& t : threads) {
        if (t.joinable())
            t.join();
    }
}

void Threads::Clear()
{
    lock_guard<mutex> lock{ jobsMutex };
    while (!jobs.empty())
        jobs.pop();
}

void Threads::threadTask(Threads* pool)
{
    while (!pool->terminate) {
        unique_lock<mutex> jobsLock{ pool->jobsMutex };

        if (pool->jobs.empty())
            pool->jobsAvailable.wait(jobsLock, [&]() { return pool->terminate || !(pool->jobs.empty()); });

        if (!pool->terminate) {
            auto job = move(pool->jobs.front());
            pool->jobs.pop();
            jobsLock.unlock();
            job();
        }
    }
}
