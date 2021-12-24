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

    using read_callback = std::function<int (void *data)>;
    using write_callback = std::function<int (void *data)>;
    using error_callback = std::function<void (void *data, const char *err_msg)>;

    int fd;
    int events;   //表示event类型

    read_callback eventReadCallback;
    write_callback eventWriteCallback;
    error_callback error_fn;
    void *data; //callback data, 可能是event_loop，也可能是tcp_server或者tcp_connection
    
    channel(int fd, int events) 
        : fd(fd)
        , events(events)
    {
    }

    channel(int fd, int events, read_callback read_cb, write_callback write_cb, void *data) 
        : fd(fd)
        , events(events)
        , eventReadCallback(read_cb)
        , eventWriteCallback(write_cb)
        , data(data)
    {
    }

    channel* read(read_callback read_fn) {
        eventReadCallback = std::move(read_fn);
        return this;
    }
    channel* write(write_callback write_fn) {
        eventWriteCallback = std::move(write_fn);
        return this;
    }
    channel* error(error_callback err_fn) {
        error_fn = std::move(err_fn);
        return this;
    }

    int is_enabled_write() const {
        return events & EVENT_WRITE;
    }
    int enable_write();
    int disable_write();
};

using channel_map = std::map<int, channel *>;

// } // namespace yolandapp

#endif // YOLANDAPP_CHANNEL_H
