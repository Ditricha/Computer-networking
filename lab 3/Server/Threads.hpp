#ifndef DBR_CC_THREAD_POOL_HPP
#define DBR_CC_THREAD_POOL_HPP

#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <future>
#include <condition_variable>
#include <queue>

using namespace std;

class Threads
{
public:

    Threads(size_t threadCount = 0);

    Threads(const ThreadPool&) = delete;
    Threads& operator=(const Threads&) = delete;

    Threads(Threads&&) = default;
    Threads& operator=(Threads&&) = default;

    ~Threads();

    template<typename Func, typename... Args>
    auto add(Func&& func, Args&&... args)->future<typename result_of<Func(Args...)>::type>;

    void Clear();

private:
    using Job = function<void()>;

    static void threadTask(Threads* pool);

    queue<Job> jobs;
    mutable mutex jobsMutex;

    condition_variable jobsAvailable;

    vector<thread> threads;

    atomic<bool> terminate;
};

template<typename Func, typename... Args>
auto Threads::add(Func&& func, Args&&... args)->future<typename result_of<Func(Args...)>::type>
{
    using PackedTask = packaged_task<typename result_of<Func(Args...)>::type()>;

    auto task = make_shared<PackedTask>(bind(forward<Func>(func), forward<Args>(args)...));

    auto ret = task->get_future();

    lock_guard<mutex> lock{ jobsMutex };
    jobs.emplace([task]() { (*task)(); });

    jobsAvailable.notify_one();
    return ret;
}

#endif
