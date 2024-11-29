#include "thread_pool.h"

thread_pool::thread_pool(size_t numThreads) : stop(false) {
    for(size_t i = 0;i<numThreads;++i)
        workers.emplace_back(
            [this]
            {
                while(true)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->stop.load() || !this->tasks.empty(); });
                        if(this->stop.load() && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            }
        );
}

// Destructor joins all threads
thread_pool::~thread_pool()
{
    stop.store(true);
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}
