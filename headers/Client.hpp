#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
private:
    int _fd;
    std::string _buffer;

public:
    Client();
    Client(int fd);
    Client(const Client &src);
    Client &operator=(const Client &src);
    ~Client();

    int getFd() const;
    std::string& getBuffer();
};

#endif