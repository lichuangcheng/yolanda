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
    //get the lock
    pthread_mutex_lock(&mutex);
    is_handle_pending = 1;

    struct channel_element *channelElement = pending_head;
    while (channelElement != NULL) {
        //save into event_map
        struct channel *channel = channelElement->channel;
        int fd = channel->fd;
        if (channelElement->type == 1) {
            handle_pending_add(fd, channel);
        } else if (channelElement->type == 2) {
            handle_pending_remove(fd, channel);
        } else if (channelElement->type == 3) {
            handle_pending_update(fd, channel);
        }
        channelElement = channelElement->next;
    }

    pending_head = pending_tail = NULL;
    is_handle_pending = 0;

    //release the lock
    pthread_mutex_unlock(&mutex);

    return 0;
}

void event_loop::channel_buffer_nolock(int fd, struct channel *channel1, int type) {
    //add channel into the pending list
    struct channel_element *channelElement = (struct channel_element *)malloc(sizeof(struct channel_element));
    channelElement->channel = channel1;
    channelElement->type = type;
    channelElement->next = NULL;
    //第一个元素
    if (this->pending_head == NULL) {
        this->pending_head = this->pending_tail = channelElement;
    } else {
        this->pending_tail->next = channelElement;
        this->pending_tail = channelElement;
    }
}

int event_loop::do_channel_event(int fd, struct channel *channel1, int type) {
    //get the lock
    pthread_mutex_lock(&mutex);
    assert(is_handle_pending == 0);
    channel_buffer_nolock(fd, channel1, type);
    //release the lock
    pthread_mutex_unlock(&mutex);
    if (!isInSameThread(this)) {
        wakeup();
    } else {
        handle_pending_channel();
    }

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

    if (!map.contains(fd))
        return (-1);

    struct channel *channel2 = map[fd];

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

    if (!map.contains(fd)) return (-1);

    channel *channel = map[fd];
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
    yolanda_msgx("wakeup, %s", eventLoop->thread_name);
    return 0;
}

event_loop::event_loop() 
    : event_loop("")
{
}

event_loop::~event_loop()
{
}

event_loop::event_loop(const std::string &thread_name) 
{
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    this->thread_name = thread_name.empty() ? "main thread" : thread_name;
    quit = 0;

    yolanda_msgx("set epoll as dispatcher, %s", this->thread_name.c_str());
    eventDispatcher = std::make_shared<event_dispatcher>();
    eventDispatcher->init();

    //add the socketfd to event
    owner_thread_id = pthread_self();
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketPair) < 0) {
        YOLONDA_LOG_ERR("socketpair set fialed");
    }
    is_handle_pending = 0;
    pending_head = NULL;
    pending_tail = NULL;
    
    // TODO: 释放 channel 内存
    auto chan = new channel(socketPair[1], EVENT_READ, handleWakeup, NULL, this);
    this->add_channel_event(chan->fd, chan);
}

/**
 *
 * 1.参数验证
 * 2.调用dispatcher来进行事件分发,分发完回调事件处理函数
 */
int event_loop::run() 
{
    struct event_dispatcher *dispatcher = eventDispatcher.get();
    if (this->owner_thread_id != pthread_self()) {
        exit(1);
    }

    yolanda_msgx("event loop run, %s", this->thread_name.c_str());
    struct timeval timeval;
    timeval.tv_sec = 1;

    while (!this->quit) {
        //block here to wait I/O event, and get active channels
        dispatcher->dispatch(this, &timeval);

        //handle the pending channel
        handle_pending_channel();
    }

    yolanda_msgx("event loop end, %s", this->thread_name);
    return 0;
}


