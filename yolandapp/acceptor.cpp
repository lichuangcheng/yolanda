#include <assert.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "yolandapp/acceptor.hpp"
#include "yolandapp/log.hpp"

void set_blocking(int fd, bool flag)
{
    int arg = fcntl(fd, F_GETFL);
    long flags = arg & ~O_NONBLOCK;
    if (!flag)
        flags |= O_NONBLOCK;
    (void)fcntl(fd, F_SETFL, flags);
}

void make_nonblocking(int fd) {
    set_blocking(fd, false);
}

acceptor::acceptor(int port) 
{
    listen_port = port;
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    make_nonblocking(listen_fd);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int on = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    int rt1 = bind(listen_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (rt1 < 0) {
        error(1, errno, "bind failed ");
    }

    int rt2 = listen(listen_fd, 1024);
    if (rt2 < 0) {
        error(1, errno, "listen failed ");
    }

    // signal(SIGPIPE, SIG_IGN);
}

acceptor::~acceptor()
{
    if (listen_fd != -1)
        close(listen_fd);
}
