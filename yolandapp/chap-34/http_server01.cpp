#include <yolandapp/http_server.hpp>
#include <yolandapp/event_loop.hpp>
#include <yolandapp/log.hpp>
#include <string.h>

//数据读到buffer之后的callback
int onRequest(struct http_request *httpRequest, struct http_response *httpResponse) {
    auto &url = httpRequest->url;
    auto question = url.find('?');
    auto path = url.substr(0, question);

    if (path == "/") {
        httpResponse->statusCode = OK;
        httpResponse->statusMessage = "OK";
        httpResponse->contentType = "text/html";
        httpResponse->body = "<html><head><title>This is network programming</title></head><body><h1>Hello, network programming</h1></body></html>";
    } else if (strcmp(path.c_str(), "/network") == 0) {
        httpResponse->statusCode = OK;
        httpResponse->statusMessage = "OK";
        httpResponse->contentType = "text/plain";
        httpResponse->body = "hello, network programming";
    } else {
        httpResponse->statusCode = NotFound;
        httpResponse->statusMessage = "Not Found";
        httpResponse->keep_connected = 1;
    }

    return 0;
}

int main(int argc, char **argv) 
{
    int port = 8080;
    if (argc >= 2)
        port = std::stoi(argv[1]);
    
    // 主线程event_loop
    struct event_loop loop;

    // 初始tcp_server，可以指定线程数目，如果线程是0，就是在这个线程里acceptor+i/o；如果是1，有一个I/O线程
    auto httpServer = std::make_unique<http_server>(&loop, port, &onRequest, 2);
    httpServer->start();

    // main thread for acceptor
    loop.run();
}
