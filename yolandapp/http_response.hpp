#ifndef YOLANDAPP_HTTP_RESPONSE_H
#define YOLANDAPP_HTTP_RESPONSE_H

#include "yolandapp/buffer.hpp"
#include <map>
#include <string>

enum HttpStatusCode {
    Unknown,
    OK = 200,
    MovedPermanently = 301,
    BadRequest = 400,
    NotFound = 404,
};

struct http_response {
    
    using Headers = std::map<std::string, std::string>;

    enum HttpStatusCode statusCode {Unknown};
    std::string statusMessage;
    std::string contentType;
    std::string body;
    Headers headers;
    int keep_connected {0};

    http_response() = default;
    ~http_response() = default;

    void encode_buffer(buffer *buffer);
};

#endif // YOLANDAPP_HTTP_RESPONSE_H
