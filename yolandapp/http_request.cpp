#include "yolandapp/http_request.hpp"

#define INIT_REQUEST_HEADER_SIZE 128

const std::string HTTP10 = "HTTP/1.0";
const std::string HTTP11 = "HTTP/1.1";
const std::string KEEP_ALIVE = "Keep-Alive";
const std::string CLOSE = "close";

//初始化一个request对象
http_request::http_request()
{
}

http_request::~http_request()
{
}

//清除一个request对象
void http_request::clear()
{
    headers.clear();
    version.clear();
    method.clear();
    url.clear();
    cur_state = REQUEST_STATUS;
}

//重置一个request对象
void http_request::reset()
{
    clear();
}

//给request增加header
void http_request::add_header(const char *key, const char *value)
{
    headers[key] = value;
}

//根据key值获取header熟悉
const char *http_request::get_header(const char *key) const
{
    auto it = headers.find(key);
    if (it != headers.end())
        return it->second.c_str();
    return nullptr;
}

//获得request解析的当前状态
enum http_request_state http_request::current_state() const
{
    return cur_state;
}

//根据request请求判断是否需要关闭服务器-->客户端单向连接
int http_request::close_connection() const
{
    auto connection = get_header("Connection");

    if (connection != nullptr && CLOSE.compare(connection) == 0)
        return 1;

    if (version == HTTP10 && KEEP_ALIVE.compare(connection) != 0)
        return 1;

    return 0;
}
