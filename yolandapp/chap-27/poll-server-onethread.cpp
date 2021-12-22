#include <yolandapp/acceptor.hpp>
#include <yolandapp/event_loop.hpp>
#include <yolandapp/tcp_server.hpp>

char rot13_char(char c) {
    if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
        return c + 13;
    else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
        return c - 13;
    else
        return c;
}

//连接建立之后的callback
int onConnectionCompleted(struct tcp_connection *tcpConnection) {
    printf("connection completed\n");
    return 0;
}

//数据读到buffer之后的callback
int onMessage(struct buffer *input, struct tcp_connection *tcpConnection) {
    printf("get message from tcp connection %s\n", tcpConnection->name.c_str());
    printf("%s", input->data.c_str());

    buffer output;
    int size = input->readable();
    for (int i = 0; i < size; i++) {
        output.append(rot13_char(input->read_char()));
    }
    tcpConnection->send_buffer(&output);
    return 0;
}

//数据通过buffer写完之后的callback
int onWriteCompleted(struct tcp_connection *tcpConnection) {
    printf("write completed\n");
    return 0;
}

//连接关闭之后的callback
int onConnectionClosed(struct tcp_connection *tcpConnection) {
    printf("connection closed\n");
    return 0;
}

int main(int c, char **v) {
    //主线程event_loop
    event_loop ev;

    //初始化acceptor
    acceptor acceptor(8080);

    //初始tcp_server，可以指定线程数目，如果线程是0，就只有一个线程，既负责acceptor，也负责I/O
    TCPserver tcpServer(&ev, &acceptor, onConnectionCompleted, onMessage,
                        onWriteCompleted, onConnectionClosed, 0);
    tcpServer.start();

    // main thread for acceptor
    ev.run();
}
