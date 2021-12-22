#ifndef YOLANDAPP_HTTP_SERVER_H
#define YOLANDAPP_HTTP_SERVER_H

#include "yolandapp/acceptor.hpp"
#include "yolandapp/tcp_server.hpp"
#include "yolandapp/http_request.hpp"
#include "yolandapp/http_response.hpp"
#include <functional>

using request_callback = std::function<int (http_request *httpRequest, http_response *httpResponse)>;

struct http_server {
    request_callback requestCallback;
    struct acceptor acceptor;
    struct TCPserver tcpServer;

    http_server(event_loop *eventLoop, int port, request_callback requestCallback, int threadNum);
    ~http_server();

    void start();
};

int parse_http_request(struct buffer *input, struct http_request *httpRequest);

#endif
