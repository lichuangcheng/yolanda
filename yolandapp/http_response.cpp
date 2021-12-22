#include "yolandapp/http_response.hpp"
#include <string.h>

void http_response::encode_buffer(buffer *output)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "HTTP/1.1 %d ", statusCode);
    output->append(buf, strlen(buf));
    output->append(statusMessage);
    output->append("\r\n");

    if (keep_connected)
    {
        output->append("Connection: close\r\n");
    }
    else
    {
        snprintf(buf, sizeof(buf), "Content-Length: %lu\r\n", body.size());
        output->append(buf, strlen(buf));
        output->append("Connection: Keep-Alive\r\n");
    }

    for (auto &[key, value] : headers)
    {
        output->append(key);
        output->append(": ");
        output->append(value);
        output->append("\r\n");
    }

    output->append("\r\n");
    output->append(body);
}
