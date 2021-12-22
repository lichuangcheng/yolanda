#ifndef YOLANDAPP_EVENT_LOOP_H
#define YOLANDAPP_EVENT_LOOP_H

#include <pthread.h>
#include "yolandapp/channel.hpp"
#include "yolandapp/event_dispatcher.hpp"
#include <memory>


struct channel_element {
    int type; //1: add  2: delete
    struct channel *channel;
    struct channel_element *next;
};

struct event_loop {
    int quit;
    std::shared_ptr<event_dispatcher> eventDispatcher;

    /** 对应的event_dispatcher的数据. */
    void *event_dispatcher_data;
    channel_map channelMap;

    int is_handle_pending;
    struct channel_element *pending_head;
    struct channel_element *pending_tail;

    pthread_t owner_thread_id;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int socketPair[2];
    std::string thread_name;

    event_loop();
    event_loop(const std::string &thread_name);

    int run();

    // 唤醒
    void wakeup();

    // 增加通道事件
    int add_channel_event(int fd, struct channel *channel1);

    int remove_channel_event(int fd, struct channel *channel1);

    int update_channel_event(int fd, struct channel *channel1);

    int handle_pending_add(int fd, struct channel *channel);

    int handle_pending_remove(int fd, struct channel *channel);

    int handle_pending_update(int fd, struct channel *channel);


    // dispather派发完事件之后，调用该方法通知event_loop执行对应事件的相关callback方法
    // res: EVENT_READ | EVENT_READ等
    int channel_event_activate(int fd, int res);

    ~event_loop();

private:
    int handle_pending_channel();
    int do_channel_event(int fd, struct channel *channel1, int type);
    void channel_buffer_nolock(int fd, struct channel *channel1, int type);
};

void assertInSameThread(struct event_loop *eventLoop);

//1： same thread: 0： not the same thread
int isInSameThread(struct event_loop *eventLoop);


#endif
