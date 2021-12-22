#ifndef YOLANDAPP_CHANNEL_H
#define YOLANDAPP_CHANNEL_H

#include <functional>
#include <map>
#include "yolandapp/buffer.hpp"

#define EVENT_TIMEOUT    0x01
/** Wait for a socket or FD to become readable */
#define EVENT_READ        0x02
/** Wait for a socket or FD to become writeable */
#define EVENT_WRITE    0x04
/** Wait for a POSIX signal to be raised*/
#define EVENT_SIGNAL    0x08

// namespace yolandapp {


class channel 
{
public:
    enum Event {
        TIMEOUT = 0x01,
        READ    =  0x02,
        WRITE   = 0x04,
        SIGNAL  = 0x08
    };

    using event_read_callback = std::function<int (void *data)>;
    using event_write_callback = std::function<int (void *data)>;

    int fd;
    int events;   //表示event类型

    event_read_callback eventReadCallback;
    event_write_callback eventWriteCallback;
    void *data; //callback data, 可能是event_loop，也可能是tcp_server或者tcp_connection
    
    channel(int fd, int events, event_read_callback read_cb, event_write_callback write_cb, void *data) {
        this->fd = fd;
        this->events = events;
        this->eventReadCallback = read_cb;
        this->eventWriteCallback = write_cb;
        this->data = data;
    }

    int write_event_is_enabled() const {
        return events & EVENT_WRITE;
    }

    int write_event_enable();
    
    int write_event_disable();
};

using channel_map = std::map<int, channel *>;


int channel_write_event_is_enabled(struct channel *channel);

int channel_write_event_enable(struct channel *channel);

int channel_write_event_disable(struct channel *channel);


// } // namespace yolandapp

#endif // YOLANDAPP_CHANNEL_H
