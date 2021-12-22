#ifndef YOLANDAPP_TCP_SERVER_H
#define YOLANDAPP_TCP_SERVER_H

#include "yolandapp/acceptor.hpp"
#include "yolandapp/event_loop.hpp"
#include "yolandapp/thread_pool.hpp"
#include "yolandapp/buffer.hpp"
#include "yolandapp/tcp_connection.hpp"


struct TCPserver {
    int port;
    struct event_loop *eventLoop;
    struct acceptor *acceptor;
    connection_completed_call_back connectionCompletedCallBack;
    message_call_back messageCallBack;
    write_completed_call_back writeCompletedCallBack;
    connection_closed_call_back connectionClosedCallBack;
    int threadNum;
    std::unique_ptr<thread_pool> threadPool;
    void * data; //for callback use: http_server

    std::shared_ptr<channel> chan;

    TCPserver(struct event_loop *eventLoop, struct acceptor *acceptor,
                connection_completed_call_back connectionCallBack,
                message_call_back messageCallBack,
                write_completed_call_back writeCompletedCallBack,
                connection_closed_call_back connectionClosedCallBack,
                int threadNum);

    //开启监听
    void start();

    //设置callback数据
    void set_data(void * data);
};


// export 
int tcp_server(int port);
int tcp_server_listen(int port);
int tcp_nonblocking_server_listen(int port);

#endif
