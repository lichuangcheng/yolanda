#include <assert.h>
#include "yolandapp/thread_pool.hpp"

thread_pool::thread_pool(event_loop *mainLoop, int threadNumber)
{

    this->mainLoop = mainLoop;
    this->position = 0;
    this->thread_number = threadNumber;
    eventLoopThreads.reserve(threadNumber);
    this->started = 0;
}

//一定是main thread发起
void thread_pool::start()
{
    assert(!started);
    assertInSameThread(mainLoop);

    started = 1;
    if (thread_number <= 0) 
        return;

    for (int i = 0; i < thread_number; i++)
        eventLoopThreads.emplace_back(new event_loop_thread(i))->start();
}

//一定是main thread中选择
struct event_loop *thread_pool::get_loop()
{
    assert(started);
    assertInSameThread(mainLoop);

    //优先选择当前主线程
    struct event_loop *selected = mainLoop;

    //从线程池中按照顺序挑选出一个线程
    if (thread_number > 0)
    {
        selected = eventLoopThreads[position]->eventLoop.get();
        if (++position >= thread_number)
        {
            position = 0;
        }
    }

    return selected;
}
