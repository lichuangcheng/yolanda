#include "yolandapp/utils.hpp"
#include "yolandapp/log.hpp"
#include "yolandapp/event_loop.hpp"


void assertInSameThread(struct event_loop *eventLoop) {
    if (eventLoop->owner_thread_id != pthread_self()) {
        YOLONDA_LOG_ERR("not in the same thread");
        exit(-1);
    }
}

//1： same thread: 0： not the same thread
int isInSameThread(struct event_loop *eventLoop){
    return eventLoop->owner_thread_id == pthread_self();
}
