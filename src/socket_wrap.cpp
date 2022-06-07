#include "socket_wrap.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

#define INVALID_VALUE -1

// SocketWrap::SocketWrap() : socket_fd(INVALID_VALUE)
//     {}

SocketWrap::SocketWrap(int socket_desc)
{
    if(isfdtype(socket_desc, S_IFSOCK) == 1)
        socket_fd = socket_desc;
    else   
        socket_fd = INVALID_VALUE;
}

SocketWrap::SocketWrap(int domain, int type, int protocol /*=0*/)
{
    socket_fd = socket(domain, type, protocol);
}

SocketWrap::~SocketWrap()
{
    if(socket_fd != INVALID_VALUE)
        close(socket_fd);
}
