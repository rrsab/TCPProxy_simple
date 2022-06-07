#ifndef SOCKET_WRAP_H
#define SOCKET_WRAP_H

class SocketWrap
{
public:
    // SocketWrap();
    SocketWrap(int socket_fd);
    SocketWrap(int domain, int type, int protocol = 0);
    virtual ~SocketWrap();
    int get_socket_fd() { return socket_fd; }

private:
    int socket_fd;

    //Lock copy
    SocketWrap(const SocketWrap&) = delete;
    SocketWrap& operator=(const SocketWrap&) = delete;
};

#endif
