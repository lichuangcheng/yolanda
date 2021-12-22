#ifndef YOLANDAPP_ACCEPTOR_H
#define YOLANDAPP_ACCEPTOR_H

struct acceptor{
    int listen_port;
    int listen_fd;

    acceptor(int port);
    ~acceptor();
};

void set_blocking(int fd, bool flag);
void make_nonblocking(int fd);
struct acceptor *acceptor_init(int port);

#endif
