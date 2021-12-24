#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "yolandapp/event_loop.hpp"
#include "yolandapp/log.hpp"
#include "yolandapp/event_dispatcher.hpp"
#include "yolandapp/channel.hpp"
#include "yolandapp/utils.hpp"

// in the i/o thread
int event_loop::handle_pending_channel() 
{
    // scope lock
    std::lock_guard<std::mutex> lock(mutex);
    is_handle_pending = 1;

    for (auto &pending : pending_list)
    {
        auto chan = pending.channel;
        int fd = chan->fd;
        if (pending.type == 1)
            handle_pending_add(fd, chan);
        else if (pending.type == 2)
            handle_pending_remove(fd, chan);
        else if (pending.type == 3)
            handle_pending_update(fd, chan);
    }
    pending_list.clear();
    is_handle_pending = 0;
    return 0;
}

int event_loop::do_channel_event(int fd, struct channel *channel1, int type) 
{
    {
        // scope lock
        std::lock_guard<std::mutex> lock(mutex);
        assert(is_handle_pending == 0);
         // 将通道事件入队
        pending_list.push_back(channel_element{type, channel1, nullptr});
    }

    if (!isInSameThread(this)) 
        wakeup();
    else 
        handle_pending_channel();

    return 0;
}

int event_loop::add_channel_event(int fd, channel *channel1) {
    return do_channel_event(fd, channel1, 1);
}

int event_loop::remove_channel_event(int fd, channel *channel1) {
    return do_channel_event(fd, channel1, 2);
}

int event_loop::update_channel_event(int fd, channel *channel1) {
    return do_channel_event(fd, channel1, 3);
}

// in the i/o thread
int event_loop::handle_pending_add(int fd, channel *channel) {
    yolanda_msgx("add channel fd == %d, %s", fd, this->thread_name.c_str());
    auto &map = this->channelMap;

    if (fd < 0)
        return 0;

    //第一次创建，增加
    if (!map.contains(fd)) {

        map[fd] = channel;
        //add channel
        eventDispatcher->add(channel);
        return 1;
    }
    return 0;
}

// in the i/o thread
int event_loop::handle_pending_remove(int fd, struct channel *channel1) {
    auto &map = this->channelMap;
    assert(fd == channel1->fd);

    if (fd < 0)
        return 0;

    auto it = map.find(fd);
    if (it == map.end()) return -1;
    
    struct channel *channel2 = it->second;

    //update dispatcher(multi-thread)here
    int retval = 0;
    if (eventDispatcher->del(channel2) == -1) {
        retval = -1;
    } else {
        retval = 1;
    }

    map.erase(fd);
    return retval;
}

// in the i/o thread
int event_loop::handle_pending_update(int fd, struct channel *channel) {
    yolanda_msgx("update channel fd == %d, %s", fd, thread_name);
    auto &map = this->channelMap;

    if (fd < 0)
        return 0;

    if (!map.contains(fd))
        return (-1);

    //update channel
    eventDispatcher->update(channel);
    return 0;
}

int event_loop::channel_event_activate(int fd, int revents) {
    channel_map &map = channelMap;
    yolanda_msgx("activate channel fd == %d, revents=%d, %s", fd, revents, thread_name.c_str());

    if (fd < 0)
        return 0;

    auto it = map.find(fd);
    if (it == map.end()) return -1;

    channel *channel = it->second;
    assert(fd == channel->fd);

    if (revents & (EVENT_READ)) {
        if (channel->eventReadCallback) channel->eventReadCallback(channel->data);
    }
    if (revents & (EVENT_WRITE)) {
        if (channel->eventWriteCallback) channel->eventWriteCallback(channel->data);
    }

    return 0;
}

void event_loop::wakeup() {
    char one = 'a';
    ssize_t n = write(this->socketPair[0], &one, sizeof one);
    if (n != sizeof one) {
        YOLONDA_LOG_ERR("wakeup event loop thread failed");
    }
}

int handleWakeup(void *data) {
    struct event_loop *eventLoop = (struct event_loop *) data;
    char one;
    ssize_t n = read(eventLoop->socketPair[1], &one, sizeof one);
    if (n != sizeof(one)) {
        YOLONDA_LOG_ERR("handleWakeup  failed");
    }
    yolanda_msgx("wakeup, %s", eventLoop->thread_name.c_str());
    return 0;
}

event_loop::event_loop() 
    : event_loop("")
{
}

event_loop::~event_loop()
{
    close(socketPair[0]);
    close(socketPair[1]);
}

event_loop::event_loop(const std::string &thread_name) 
{
    this->thread_name = thread_name.empty() ? "main thread" : thread_name;
    quit = 0;

    yolanda_msgx("set epoll as dispatcher, %s", this->thread_name.c_str());
    eventDispatcher = std::make_shared<event_dispatcher>();

    //add the socketfd to event
    owner_thread_id = pthread_self();
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketPair) < 0) {
        YOLONDA_LOG_ERR("socketpair set fialed");
    }
    is_handle_pending = 0;
    pending_list.reserve(64);
    
    wakeup_channel.reset(new channel(socketPair[1], EVENT_READ, handleWakeup, NULL, this));
    this->add_channel_event(wakeup_channel->fd, wakeup_channel.get());
}

/**
 *
 * 1.参数验证
 * 2.调用dispatcher来进行事件分发,分发完回调事件处理函数
 */
int event_loop::run() 
{
    if (this->owner_thread_id != pthread_self()) {
        exit(1);
    }

    yolanda_msgx("event loop run, %s", this->thread_name.c_str());
    struct timeval timeval;
    timeval.tv_sec = 1;

    while (!this->quit) {
        //block here to wait I/O event, and get active channels
        eventDispatcher->dispatch(this, &timeval);

        //handle the pending channel
        handle_pending_channel();
    }

    yolanda_msgx("event loop end, %s", this->thread_name);
    return 0;
}

void event_loop::stop() 
{
    quit = 1;
    if (!isInSameThread(this));
        wakeup();
}


