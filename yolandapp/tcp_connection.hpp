#ifndef YOLANDAPP_TCP_CONNECTION
#define YOLANDAPP_TCP_CONNECTION

#include "yolandapp/event_loop.hpp"
#include "yolandapp/channel.hpp"
#include "yolandapp/buffer.hpp"

// using yolandapp::buffer;
struct tcp_connection;

typedef int (*connection_completed_call_back)(struct tcp_connection *tcpConnection);

typedef int (*message_call_back)(struct buffer *buffer, struct tcp_connection *tcpConnection);

typedef int (*write_completed_call_back)(struct tcp_connection *tcpConnection);

typedef int (*connection_closed_call_back)(struct tcp_connection *tcpConnection);

struct tcp_connection {
    event_loop *eventLoop;
    std::shared_ptr<channel> chan;
    std::string name;
    buffer input_buffer;   //接收缓冲区
    buffer output_buffer;  //发送缓冲区

    connection_completed_call_back connectionCompletedCallBack;
    message_call_back messageCallBack;
    write_completed_call_back writeCompletedCallBack;
    connection_closed_call_back connectionClosedCallBack;

    void * data; //for callback use: http_server
    void * request; // for callback use
    void * response; // for callback use

    tcp_connection(int fd, struct event_loop *eventLoop, connection_completed_call_back connectionCompletedCallBack,
                   connection_closed_call_back connectionClosedCallBack,
                   message_call_back messageCallBack, write_completed_call_back writeCompletedCallBack);

    int send_data(void *data, size_t size);
    int send_buffer(buffer *buffer);
    void shutdown();
};

#endif
