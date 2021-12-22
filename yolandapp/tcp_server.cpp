#include <assert.h>
#include <sys/fcntl.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "yolandapp/acceptor.hpp"
#include "yolandapp/log.hpp"
#include "yolandapp/tcp_server.hpp"
#include "yolandapp/thread_pool.hpp"
#include "yolandapp/event_loop.hpp"


int tcp_server(int port) 
{
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    int rt1 = bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (rt1 < 0) {
        error(1, errno, "bind failed ");
    }

    int rt2 = listen(listenfd, 1024);
    if (rt2 < 0) {
        error(1, errno, "listen failed ");
    }

    signal(SIGPIPE, SIG_IGN);

    int connfd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    if ((connfd = accept(listenfd, (struct sockaddr *) &client_addr, &client_len)) < 0) {
        error(1, errno, "bind failed ");
    }

    return connfd;
}


int tcp_server_listen(int port) {
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    int rt1 = bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (rt1 < 0) {
        error(1, errno, "bind failed ");
    }

    int rt2 = listen(listenfd, 1024);
    if (rt2 < 0) {
        error(1, errno, "listen failed ");
    }

    signal(SIGPIPE, SIG_IGN);

    return listenfd;
}


int tcp_nonblocking_server_listen(int port) {
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    make_nonblocking(listenfd);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    int rt1 = bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (rt1 < 0) {
        error(1, errno, "bind failed ");
    }

    int rt2 = listen(listenfd, 1024);
    if (rt2 < 0) {
        error(1, errno, "listen failed ");
    }

    signal(SIGPIPE, SIG_IGN);

    return listenfd;
}


TCPserver::TCPserver(struct event_loop *eventLoop, struct acceptor *acceptor,
                connection_completed_call_back connectionCompletedCallBack,
                message_call_back messageCallBack,
                write_completed_call_back writeCompletedCallBack,
                connection_closed_call_back connectionClosedCallBack,
                int threadNum) {
    this->eventLoop = eventLoop;
    this->acceptor = acceptor;
    this->connectionCompletedCallBack = connectionCompletedCallBack;
    this->messageCallBack = messageCallBack;
    this->writeCompletedCallBack = writeCompletedCallBack;
    this->connectionClosedCallBack = connectionClosedCallBack;
    this->threadNum = threadNum;
    this->threadPool = std::make_unique<thread_pool>(eventLoop, threadNum);
    this->data = NULL;
}


int handle_connection_established(void *data) {
    struct TCPserver *tcpServer = (struct TCPserver *) data;
    struct acceptor *acceptor = tcpServer->acceptor;
    int listenfd = acceptor->listen_fd;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int connected_fd = accept(listenfd, (struct sockaddr *) &client_addr, &client_len);
    make_nonblocking(connected_fd);

    yolanda_msgx("new connection established, socket == %d", connected_fd);

    // choose event loop from the thread pool
    struct event_loop *eventLoop = tcpServer->threadPool->get_loop();

    // create a new tcp connection
    // TODO: 释放内存（...）
    auto tcpConnection = new tcp_connection(connected_fd, eventLoop,
                                            tcpServer->connectionCompletedCallBack,
                                            tcpServer->connectionClosedCallBack,
                                            tcpServer->messageCallBack,
                                            tcpServer->writeCompletedCallBack);
    //for callback use
    if (tcpServer->data != NULL) {
        tcpConnection->data = tcpServer->data;
    }
    return 0;
}


//开启监听
void TCPserver::start() {
    //开启多个线程
    threadPool->start();

    //acceptor主线程， 同时把tcpServer作为参数传给channel对象
    chan = std::make_shared<channel>(acceptor->listen_fd, EVENT_READ, handle_connection_established, nullptr,
                                          this);
    eventLoop->add_channel_event(chan->fd, chan.get());
    return;
}

//设置callback数据
void TCPserver::set_data(void *data) {
    if (data != nullptr) {
        this->data = data;
    }
}
