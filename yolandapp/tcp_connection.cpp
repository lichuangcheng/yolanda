#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "yolandapp/utils.hpp"
#include "yolandapp/log.hpp"
#include "yolandapp/tcp_connection.hpp"
#include "yolandapp/buffer.hpp"
#include "yolandapp/event_loop.hpp"

// using yolandapp::buffer;

int handle_connection_closed(tcp_connection *tcpConnection) {
    event_loop *eventLoop = tcpConnection->eventLoop;
    auto channel = tcpConnection->chan.get();
    eventLoop->remove_channel_event(channel->fd, channel);

    if (tcpConnection->connectionClosedCallBack != NULL) {
        tcpConnection->connectionClosedCallBack(tcpConnection);
    }
    return 0;
}

int handle_read(void *data) {
    tcp_connection *tcpConnection = (tcp_connection *) data;
    auto *input_buffer = &tcpConnection->input_buffer;
    channel *channel = tcpConnection->chan.get();

    if (input_buffer->socket_read(channel->fd) > 0) {
        //应用程序真正读取Buffer里的数据
        if (tcpConnection->messageCallBack != NULL) {
            tcpConnection->messageCallBack(input_buffer, tcpConnection);
        }
    } else {
        handle_connection_closed(tcpConnection);
    }
    return 0;
}

//发送缓冲区可以往外写
//把channel对应的output_buffer不断往外发送
int handle_write(void *data) {
    tcp_connection *tcpConnection = (tcp_connection *) data;
    event_loop *eventLoop = tcpConnection->eventLoop;
    assertInSameThread(eventLoop);

    struct buffer *output_buffer = &tcpConnection->output_buffer;
    struct channel *channel = tcpConnection->chan.get();

    ssize_t nwrited = write(channel->fd, output_buffer->data.data() + output_buffer->readIndex,
                            output_buffer->readable());
    if (nwrited > 0) {
        //已读nwrited字节
        output_buffer->readIndex += nwrited;
        //如果数据完全发送出去，就不需要继续了
        if (output_buffer->readable() == 0) {
            channel->write_event_disable();
        }
        //回调writeCompletedCallBack
        if (tcpConnection->writeCompletedCallBack != NULL) {
            tcpConnection->writeCompletedCallBack(tcpConnection);
        }
    } else {
        yolanda_msgx("handle_write for tcp connection %s", tcpConnection->name.c_str());
    }

    return 0;
}

tcp_connection::tcp_connection(int connected_fd, struct event_loop *eventLoop,
                   connection_completed_call_back connectionCompletedCallBack,
                   connection_closed_call_back connectionClosedCallBack,
                   message_call_back messageCallBack, write_completed_call_back writeCompletedCallBack) {
    this->writeCompletedCallBack = writeCompletedCallBack;
    this->messageCallBack = messageCallBack;
    this->connectionCompletedCallBack = connectionCompletedCallBack;
    this->connectionClosedCallBack = connectionClosedCallBack;
    this->eventLoop = eventLoop;

    char buf[64];
    sprintf(buf, "connection-%d", connected_fd);
    this->name = buf;

    // add event read for the new connection
    chan = std::make_shared<struct channel>(connected_fd, EVENT_READ, handle_read, handle_write, this);

    //connectionCompletedCallBack callback
    if (this->connectionCompletedCallBack) {
        this->connectionCompletedCallBack(this);
    }

    eventLoop->add_channel_event(connected_fd, chan.get());
}

//应用层调用入口
int tcp_connection_send_data(struct tcp_connection *tcpConnection, void *data, int size) {
    ssize_t nwrited = 0;
    size_t nleft = size;
    int fault = 0;

    struct channel *channel = tcpConnection->chan.get();
    auto &output_buffer = tcpConnection->output_buffer;

    //先往套接字尝试发送数据
    
    if (!channel->write_event_is_enabled() && output_buffer.readable() == 0) {
        nwrited = write(channel->fd, data, size);
        if (nwrited >= 0) {
            nleft = nleft - nwrited;
        } else {
            nwrited = 0;
            if (errno != EWOULDBLOCK) {
                if (errno == EPIPE || errno == ECONNRESET) {
                    fault = 1;
                }
            }
        }
    }

    if (!fault && nleft > 0) {
        //拷贝到Buffer中，Buffer的数据由框架接管
        output_buffer.append(data + nwrited, nleft);
        if (!channel->write_event_is_enabled()) {
            channel->write_event_enable();
        }
    }

    return nwrited;
}

int tcp_connection::send_buffer(buffer *buffer) {
    int size = buffer->readable();
    int result = tcp_connection_send_data(this, (void *)(buffer->data.data() + buffer->readIndex), size);
    buffer->readIndex += size;
    return result;
}

void tcp_connection::shutdown() {
    if (::shutdown(chan->fd, SHUT_WR) < 0) {
        yolanda_msgx("tcp_connection_shutdown failed, socket == %d", chan->fd);
    }
}
