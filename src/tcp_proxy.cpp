#include <unistd.h>
#include <arpa/inet.h>
#include <mutex>
#include <fstream>
#include <iostream>
#include <thread>
#include <fcntl.h>


#define BUF_SIZE 8192
#define MAX_CONNECT_REQUESTS 1024

using std::cerr;
using std::cout;
using std::endl;

class tcp_proxy
{
private:
    int                 listener;
    struct sockaddr_in  addr, addr_server;
    int 				restrict = 1;
    struct sockaddr_in	clientaddr = {};
    socklen_t			len = sizeof(clientaddr);

    unsigned short      _local_port;
    unsigned short      _forward_port;
    const std::string   _local_host;
    const std::string   _forward_host;
    const std::string   _filename_log;
    char host_ip[BUF_SIZE];
    std::mutex               mtx;

std::string print_time() // определение текущего времени
{
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [80];

    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime (buffer,80,"%c",timeinfo);
    return buffer;
}

/* Вывод в лог файл принятых и отправленных данных */
void write_log(std::string source, std::string direction, char str1 [BUF_SIZE], const size_t& bytes_transferred)
{
    std::unique_lock<std::mutex> ul(mtx, std::defer_lock);
    std::ofstream output_log_;   
    std::string str;
    for (size_t i = 0; i < bytes_transferred; i++)
    {
        str += str1[i];
    }
    ul.lock();
    output_log_.open(_filename_log.c_str(), std::ios::binary | std::ios::app);
    if (!output_log_.is_open()) 
    {
        std::cerr << "Cannot open \"" << _filename_log << "\"" << std::endl;
        return ;
    }
    output_log_ << print_time() << " | " << source << " | " << direction << " | "  << str << std::endl;
    output_log_.close();
}

public:

const char *getHost_ip() const
{
    return host_ip;
}

int socket_proxy() // создание сокета прокси, "прослушивание" порта
{
    /* Объявляем сокет прокси */
    listener = socket(AF_INET, SOCK_STREAM, 0);
    
    if(listener < 0)
    {
        cerr << "Error: listener socket create" << endl;
        exit(EXIT_FAILURE);
    }
    /* Привязка сокета прокси к паре IP-адрес/Порт */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_local_port);
    addr.sin_addr.s_addr = inet_addr(_local_host.c_str());
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &restrict, sizeof(int));
    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        cerr<<"Error: listener bind"<<endl;
        exit(EXIT_FAILURE);
    }
    /* "Прослушивание" сокета */
    if (listen(listener, MAX_CONNECT_REQUESTS))
    {
        cerr << "Error: listen" << endl;
        exit(EXIT_FAILURE);
    }
    return 0;

}

int socket_client() /* установление соединения с клиентом */
{
    /* установливаем соединения с клиентом и получаем файловый дескриптор 
    соединения */
    int sock_client = accept(listener, (struct sockaddr *) &clientaddr, &len);
    if(sock_client < 0)
    {
        cerr<<"Error: accepting from client socket"<<endl;
        exit(EXIT_FAILURE);
    }
    /* сохраняем ip адрес клиента с переменную host_ip */   
    if (!(inet_ntop(AF_INET, &(clientaddr.sin_addr), host_ip, BUF_SIZE))) {
        std::cerr << "getnameinfo failure" << std::endl;
        exit(EXIT_FAILURE);
    }
    return sock_client;
}
int socket_server() /* установление соединения с сервером */
{
    /* создаем сокет для соединения с сервером, к которому будем перенаправлять
    пакеты от клиента  */
    int sock_server = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_server < 0)
    {
        cerr<<"Error: server socket create"<<endl;
        exit(EXIT_FAILURE);
    }
    /* привязка сокета для соединения с севером к его паре IP-адрес/Порт */
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(_forward_port);
    addr_server.sin_addr.s_addr = inet_addr(_forward_host.c_str());
    if(connect(sock_server, (struct sockaddr *)&addr_server, sizeof(addr_server)) < 0)
    {
        cerr << "Error: connecting to server" << endl;
        exit(EXIT_FAILURE);
    }
    return sock_server;
}

/* получение данных от клиента, отправление серверу и наоборот */
void client_processing(int sock_client, int sock_server, std::string host_ip) 
{
    char buf[BUF_SIZE];
    ssize_t bytes_read, bytes_write;
    const timespec sleep_interval{.tv_sec = 0, .tv_nsec = 1000000};

    const int cl_flags = fcntl(sock_client, F_GETFL, 0);
    fcntl(sock_client, F_SETFL, cl_flags | O_NONBLOCK);

    const int sr_flags = fcntl(sock_server, F_GETFL, 0);
    fcntl(sock_server, F_SETFL, sr_flags | O_NONBLOCK);
    
    while (true)
    {
        // получение данных от клиента и отправление серверу
        bytes_read = recv(sock_client, buf, BUF_SIZE, 0);
        if (bytes_read <= 0)
        {
            if (errno != EWOULDBLOCK) {
                std::cerr << "Error: read(sock_client)" << std::endl;
                close(sock_server);
                shutdown(sock_client, 1);
                break;
            }
        } else {
            write_log(host_ip, "client -> server", buf, bytes_read);
            bytes_write = send(sock_server, buf, bytes_read, 0);
            std::cout << "client -> server : " << bytes_read << '\\' << bytes_write << std::endl;
        }
        // получение данных от сервера и отправление клиенту
        bytes_read = recv(sock_server, buf, BUF_SIZE, 0);
        if (bytes_read <= 0)
        {
            if (errno != EWOULDBLOCK) {
                std::cerr << "Error: read(sock_server)" << std::endl;
                close(sock_server);
                shutdown(sock_client, 1);
                break;
            }
        } else {
            write_log(host_ip, "server -> client", buf, bytes_read);
            bytes_write = send(sock_client, buf, bytes_read, 0);
            std::cout << "server -> client : " << bytes_read << '\\' << bytes_write << std::endl;
        }

        nanosleep(&sleep_interval, nullptr);
    }
}

tcp_proxy(unsigned short local_port, unsigned short forward_port, std::string local_host,
            std::string forward_host, std::string filename_log)
            : _local_port(local_port), _forward_port(forward_port),
            _local_host(local_host), _forward_host(forward_host),
            _filename_log(filename_log) {}

~tcp_proxy() {}
};




int main(int argc, char* argv[])
{
    if (argc != 5)
    {
        std::cerr << "usage: tcpproxy_server <local host ip> <local port> <forward host ip> <forward port>" << std::endl;
        return 1;
    }

    const unsigned short local_port   = static_cast<unsigned short>(::atoi(argv[2]));
    const unsigned short forward_port = static_cast<unsigned short>(::atoi(argv[4]));
    const std::string local_host      = argv[1];
    const std::string forward_host    = argv[3];
    const std::string filename_log    = "tcpproxy.log";

    int sock_client, sock_server;
    std::string host_ip;

    tcp_proxy proxy(local_port, forward_port, local_host, forward_host, filename_log);
    proxy.socket_proxy();
   
    while (true)
    {
        sock_client = proxy.socket_client();
        sock_server = proxy.socket_server();
        host_ip = proxy.getHost_ip();
        std::thread thrd_client([&] ()
        {
            proxy.client_processing(sock_client, sock_server, host_ip);
        });
        thrd_client.detach();
    }
    return 0;
}