#include <sys/epoll.h>
#include <unistd.h>
#include "yolandapp/event_dispatcher.hpp"
#include "yolandapp/event_loop.hpp"
#include "yolandapp/log.hpp"


int event_dispatcher::add(struct channel *channel1) {
    int fd = channel1->fd;
    int events = 0;
    if (channel1->events & EVENT_READ) {
        events = events | EPOLLIN;
    }
    if (channel1->events & EVENT_WRITE) {
        events = events | EPOLLOUT;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
//    event.events = events | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event) == -1) {
        error(1, errno, "epoll_ctl add  fd failed");
    }

    return 0;
}

int event_dispatcher::del(struct channel *channel1) 
{
    int fd = channel1->fd;

    int events = 0;
    if (channel1->events & EVENT_READ) {
        events = events | EPOLLIN;
    }

    if (channel1->events & EVENT_WRITE) {
        events = events | EPOLLOUT;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
//    event.events = events | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, &event) == -1) {
        error(1, errno, "epoll_ctl delete fd failed");
    }

    return 0;
}

int event_dispatcher::update(struct channel *channel1) 
{
    int fd = channel1->fd;

    int events = 0;
    if (channel1->events & EVENT_READ) {
        events = events | EPOLLIN;
    }

    if (channel1->events & EVENT_WRITE) {
        events = events | EPOLLOUT;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
//    event.events = events | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_MOD, fd, &event) == -1) {
        error(1, errno, "epoll_ctl modify fd failed");
    }

    return 0;
}

int event_dispatcher::dispatch(struct event_loop *eventLoop, struct timeval *timeval) 
{
    YOLANDA_UNUSED(timeval);
    int i, n;
    n = epoll_wait(efd, events.data(), events.size(), -1);
    yolanda_msgx("epoll_wait wakeup, %s", eventLoop->thread_name.c_str());
    for (i = 0; i < n; i++) {
        if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) {
            fprintf(stderr, "epoll error\n");
            close(events[i].data.fd);
            continue;
        }

        if (events[i].events & EPOLLIN) {
            yolanda_msgx("get message channel fd==%d for read, %s", events[i].data.fd, eventLoop->thread_name.c_str());
            eventLoop->channel_event_activate(events[i].data.fd, EVENT_READ);
            
        }

        if (events[i].events & EPOLLOUT) {
            yolanda_msgx("get message channel fd==%d for write, %s", events[i].data.fd,eventLoop->thread_name.c_str());
            eventLoop->channel_event_activate(events[i].data.fd, EVENT_WRITE);
        }
    }

    return 0;
}

void event_dispatcher::clear() 
{
    events.clear();
    close(efd);
    // eventLoop->event_dispatcher_data = NULL;
    return;
}

int event_dispatcher::init() 
{
    event_count = 0;
    nfds = 0;
    realloc_copy = 0;
    efd = epoll_create1(0);
    if (efd == -1) {
        error_die("epoll create failed");
        return -1;
    }

   events.resize(128);
   return 0;
}
