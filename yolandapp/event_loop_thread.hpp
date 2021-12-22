#ifndef EVENT_LOOP_THREAD_H
#define EVENT_LOOP_THREAD_H

#include <string>
#include <thread>
#include <mutex>
#include <memory>
#include <condition_variable>
#include "yolandapp/event_loop.hpp"


struct event_loop_thread {
    std::unique_ptr<event_loop> eventLoop;
    pthread_t thread_tid{0};        /* thread ID */
    std::mutex mutex;
    std::condition_variable cond;
    std::string thread_name;
    long thread_count {0};    /* # connections handled */

    //初始化已经分配内存的event_loop_thread
    event_loop_thread(int i);

    //由主线程调用，初始化一个子线程，并且让子线程开始运行event_loop
    struct event_loop* start();
};

#endif
