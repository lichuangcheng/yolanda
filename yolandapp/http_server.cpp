#include <assert.h>
#include <string.h>
#include "yolandapp/log.hpp"
#include "yolandapp/http_server.hpp"
#include "yolandapp/tcp_connection.hpp"

//连接建立之后的callback
int http_onConnectionCompleted(struct tcp_connection *tcpConnection) {
    yolanda_msgx("connection completed");
    tcpConnection->request = new http_request();
    return 0;
}

int process_status_line(char *start, char *end, struct http_request *httpRequest) 
{
    int size = end - start;
    //method
    char *space = (char *)memmem((const void *)start, (end - start), (const void *)" ", 1);
    assert(space != NULL);
    httpRequest->method.insert(0, start, space - start);

    //url
    start = space + 1;
    space = (char *)memmem(start, end - start, " ", 1);
    assert(space != NULL);
    httpRequest->url.insert(0, start, space - start);

    //version
    start = space + 1;
    httpRequest->version.insert(0, start, end - start);
    assert(space != NULL);
    return size;
}

int parse_http_request(struct buffer *input, struct http_request *httpRequest) {
    int ok = 1;
    while (httpRequest->cur_state != REQUEST_DONE) {
        if (httpRequest->cur_state == REQUEST_STATUS) {
            char *crlf = input->find_CRLF();
            if (crlf) {
                int request_line_size = process_status_line(input->data.data() + input->readIndex, crlf, httpRequest);
                if (request_line_size) {
                    input->readIndex += request_line_size;  // request line size
                    input->readIndex += 2;  //CRLF size
                    httpRequest->cur_state = REQUEST_HEADERS;
                }
            }
        } else if (httpRequest->cur_state == REQUEST_HEADERS) {
            char *crlf = input->find_CRLF();
            if (crlf) {
                /**
                 *    <start>-------<colon>:-------<crlf>
                 */
                char *start = input->data.data() + input->readIndex;
                int request_line_size = crlf - start;
                char *colon = (char *)memmem(start, request_line_size, ": ", 2);
                if (colon != NULL) {
                    std::string key;
                    std::string value;

                    key.insert(0, start, colon - start);
                    value.insert(0, colon + 2, (crlf - colon) - 2);

                    httpRequest->headers.emplace(std::move(key), std::move(value));
                    input->readIndex += request_line_size;  //request line size
                    input->readIndex += 2;  //CRLF size
                } else {
                    //读到这里说明:没找到，就说明这个是最后一行
                    input->readIndex += 2;  //CRLF size
                    httpRequest->cur_state = REQUEST_DONE;
                }
            }
        }
    }
    return ok;
}


// buffer是框架构建好的，并且已经收到部分数据的情况下
// 注意这里可能没有收到全部数据，所以要处理数据不够的情形
int http_onMessage(buffer *input, tcp_connection *tcpConnection) 
{
    yolanda_msgx("get message from tcp connection %s", tcpConnection->name.c_str());

    struct http_request *httpRequest = (struct http_request *) tcpConnection->request;
    struct http_server *httpServer = (struct http_server *) tcpConnection->data;

    if (parse_http_request(input, httpRequest) == 0) {
        std::string error_response = "HTTP/1.1 400 Bad Request\r\n\r\n";
        tcpConnection->send_data(error_response.data(), error_response.size());
        tcpConnection->shutdown();
    }

    //处理完了所有的request数据，接下来进行编码和发送
    if (httpRequest->current_state() == REQUEST_DONE) {
        auto httpResponse = std::make_shared<http_response>();

        //httpServer暴露的requestCallback回调
        if (httpServer->requestCallback) {
            httpServer->requestCallback(httpRequest, httpResponse.get());
        }
        struct buffer buffer;
        httpResponse->encode_buffer(&buffer);
        tcpConnection->send_buffer(&buffer);

        if (httpRequest->close_connection()) {
            tcpConnection->shutdown();
        }
        httpRequest->reset();
    }
    return 0;
}

//数据通过buffer写完之后的callback
int http_onWriteCompleted(struct tcp_connection *tcpConnection) {
    yolanda_msgx("write completed: %s", tcpConnection->name.c_str());
    return 0;
}

//连接关闭之后的callback
int http_onConnectionClosed(struct tcp_connection *tcpConnection) {
    yolanda_msgx("connection closed");
    if (tcpConnection->request) {
        delete (http_request *)tcpConnection->request;
        tcpConnection->request = nullptr;
    }
    return 0;
}


http_server::http_server(event_loop *eventLoop, int port,
                         request_callback requestCallback,
                         int threadNum) 
    : requestCallback(requestCallback)
    , acceptor(port)
    , tcpServer(eventLoop, &acceptor, http_onConnectionCompleted, http_onMessage,
                                            http_onWriteCompleted,
                                            http_onConnectionClosed, threadNum)
{
    tcpServer.data = this;
}

http_server::~http_server()
{
}

void http_server::start()
{
    tcpServer.start();
}
