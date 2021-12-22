#ifndef YOLANDAPP_EVENT_DISPATCHER_H
#define YOLANDAPP_EVENT_DISPATCHER_H

#include "yolandapp/channel.hpp"
#include "sys/epoll.h"
#include <string>
#include <vector>

struct event_loop;

/** 抽象的event_dispatcher结构体，对应的实现如select,poll,epoll等I/O复用. */
class event_dispatcher 
{
public:
    /**  对应实现 */
    std::string name;

    /**  初始化函数 */
    int init();

    /** 通知dispatcher新增一个channel事件*/
    int add(channel * channel);

    /** 通知dispatcher删除一个channel事件*/
    int del(channel * channel);

    /** 通知dispatcher更新channel对应的事件*/
    int update(channel * channel);

    /** 实现事件分发，然后调用event_loop的event_activate方法执行callback*/
    int dispatch(struct event_loop *eventLoop, struct timeval *);

    /** 清除数据 */
    void clear();

private:
    int event_count;
    int nfds;
    int realloc_copy;
    int efd;
    std::vector<epoll_event> events;
};

#endif
