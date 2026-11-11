#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <string>
#include <poll.h>

class Server {
private:
    int _port;
    std::string _password;
    int _server_fd;
    std::vector<struct pollfd> _fds;

public:
    Server(int port, const std::string& password);
    ~Server();
    void init();
    void start();
};

#endif