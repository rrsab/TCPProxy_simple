#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include <cstdlib>
#include <cstddef>
#include <string>
#include <fstream>
#include <ctime>

#include <iostream>
#include <string.h>
#include <netdb.h>

#include "socket_wrap.h"
//#include "tls_parser.cpp"

#include <thread>
#include <fcntl.h>
#include <time.h>

#define BUF_SIZE 8192

using std::cerr;
using std::cout;
using std::endl;



std::string print_time()
{
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [80];

    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime (buffer,80,"%c",timeinfo);
    return buffer;
}

void write_log(char stream_socket_tmp[BUF_SIZE], std::string name_stream, char str1 [BUF_SIZE], const size_t& bytes_transferred)
    {
    std::ofstream output_log_;   
    std::string filename_log_ = "tcpproxy.log";
    //std::string name_func = boost::lexical_cast<std::string>(stream_socket_tmp.local_endpoint());
    std::string name_func = "to server";
    std::string str;
    for (size_t i = 0; i < bytes_transferred; i++)
    {
        str += str1[i];
    }

    output_log_.open(filename_log_.c_str(), std::ios::binary | std::ios::app);
    if (!output_log_.is_open()) 
    {
        std::cerr << "Cannot open \"" << filename_log_ << "\"" << std::endl;
        return ;
    }
    output_log_ << print_time() << " | " << stream_socket_tmp << " | " << name_stream << " | " << name_func << " | " << str << std::endl;
    output_log_.close();
    }

void close_sock(int sock_client1, int sock_server1)
    {
        //boost::mutex::scoped_lock lock(mutex_);
        close(sock_client1);
        close(sock_server1);        
    }

void client_processing(int sock_client, int sock_server, char host_name[BUF_SIZE])
{
    cout<<"thread id = "<<std::this_thread::get_id()<<endl;
    //SocketWrap swc(sock_client);
    char buf[BUF_SIZE];
    ssize_t bytes_read, bytes_write;
    const timespec sleep_interval{.tv_sec = 0, .tv_nsec = 10000};

    const int cl_flags = fcntl(sock_client, F_GETFL, 0);
    fcntl(sock_client, F_SETFL, cl_flags | O_NONBLOCK);

    const int sr_flags = fcntl(sock_server, F_GETFL, 0);
    fcntl(sock_server, F_SETFL, sr_flags | O_NONBLOCK);

    write(2, "8\n" , 1);
    
    while (true)
    {
        // Read from client -> server
        bytes_read = recv(sock_client, buf, BUF_SIZE, 0);
        if (bytes_read <= 0)
        {
            if (errno != EWOULDBLOCK) {
                std::cerr << "Error: read(sock_client)" << std::endl;
                //close_sock(sock_client, sock_server);
                close(sock_server);
                shutdown(sock_client, 1);
                break;
                //return ;
                //exit(EXIT_FAILURE);
            }
        } else {
            write_log(host_name, "Client->P->Server", buf, bytes_read);
            bytes_write = send(sock_server, buf, bytes_read, 0);
            std::cout << "client -> server : " << bytes_read << '\\' << bytes_write << std::endl;
        }
        // Read from server -> client

        //write(2, "9\n" , 1);

        bytes_read = recv(sock_server, buf, BUF_SIZE, 0);
        if (bytes_read <= 0)
        {
            if (errno != EWOULDBLOCK) {
                std::cerr << "Error: read(sock_server)" << std::endl;
                //close_sock(sock_client, sock_server);
                close(sock_server);
                shutdown(sock_client, 1);
                break;
                //return ;
                //exit(EXIT_FAILURE);
            }
        } else {
            write_log(host_name, "Server->P->Client", buf, bytes_read);
            bytes_write = send(sock_client, buf, bytes_read, 0);
            std::cout << "server -> client : " << bytes_read << '\\' << bytes_write << std::endl;
        }
        //write(2, "10/n" , 2);

        nanosleep(&sleep_interval, nullptr);
    }
}

int main()
{
    int listener, sock_server, sock_client;
    struct sockaddr_in addr, addr_server;
    int 				restrict = 1;
    
    write(2, "1\n" , 1);

//##########################_Make_PROXY_############################
    listener = socket(AF_INET, SOCK_STREAM, 0);
    //SocketWrap sw1(listener);
    if(listener < 0)
    {
        cerr<<"Error: listener socket create"<<endl;
        return 1;
    }

    write(2, "2\n" , 1);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(1025);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &restrict, sizeof(int));
    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        cerr<<"Error: listener bind"<<endl;
        return 1;
    }
    
    write(2, "3\n" , 1);
    
    listen(listener, 5);

//#######################_Connect_to_server_##########################
    while(true)
    {
        
        write(2, "5\n" , 1);

        struct sockaddr_in	clientaddr = {};
        socklen_t			len = sizeof(clientaddr);

        sock_client = accept(listener, (struct sockaddr *) &clientaddr, &len);
        if(sock_client < 0)
        {
            cerr<<"Error: accepting from client socket"<<endl;
            continue;
        }

        char host_ip[BUF_SIZE] = {};
        
        //if (getnameinfo((struct sockaddr *) &clientaddr, len, &host[0], BUF_SIZE, nullptr, 0, 0)) {
        if (!(inet_ntop(AF_INET, &(clientaddr.sin_addr), host_ip, BUF_SIZE))) {
            std::cerr << "getnameinfo failure" << std::endl;
            exit(EXIT_FAILURE);
        }
        write(2, "5-6/n" , 3);

        //###########################socket server############################
        sock_server = socket(AF_INET, SOCK_STREAM, 0);
        //SocketWrap sw2(sock_server);
        if(sock_server < 0)
        {
            cerr<<"Error: server socket create"<<endl;
            return 1;
        }

        write(2, "4\n" , 1);

        addr_server.sin_family = AF_INET;
        addr_server.sin_port = htons(3389);
        addr_server.sin_addr.s_addr = inet_addr("192.168.1.21");
        if(connect(sock_server, (struct sockaddr *)&addr_server, sizeof(addr_server)) < 0)
        {
            cerr<<"Error: connecting to server"<<endl;
            return 1;
        }
//###########################socket server############################

        std::thread thrd_client(client_processing, sock_client, sock_server, host_ip);
        write(2, "6\n" , 1);
        thrd_client.detach();
        write(2, "7\n" , 1);

    }

    return 0;
}
