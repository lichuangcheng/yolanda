#include <assert.h>
#include "yolandapp/event_loop_thread.hpp"
#include "yolandapp/event_loop.hpp"
#include "yolandapp/log.hpp"

void *event_loop_thread_run(void *arg) {
    struct event_loop_thread *eventLoopThread = (struct event_loop_thread *) arg;

    {
        std::unique_lock<std::mutex> lock(eventLoopThread->mutex);
        // 初始化化event loop，之后通知主线程
        eventLoopThread->eventLoop = std::make_unique<event_loop>(eventLoopThread->thread_name);
        yolanda_msgx("event loop thread init and signal, %s", eventLoopThread->thread_name.c_str());
        eventLoopThread->cond.notify_one();
    }

    //子线程event loop run
    eventLoopThread->eventLoop->run();
    return 0;
}

//初始化已经分配内存的event_loop_thread
event_loop_thread::event_loop_thread(int i) 
    : thread_name("Thread " + std::to_string(i))
{
}

event_loop_thread::~event_loop_thread()
{
    eventLoop->stop();
    if (loop_thread.joinable())
        loop_thread.join();
}

//由主线程调用，初始化一个子线程，并且让子线程开始运行event_loop
struct event_loop *event_loop_thread::start()
{
    loop_thread = std::thread(event_loop_thread_run, this);
    {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [this]{
            return this->eventLoop != nullptr;
        });
    }
    
    yolanda_msgx("event loop thread started, %s", thread_name.c_str());
    return eventLoop.get();
}
