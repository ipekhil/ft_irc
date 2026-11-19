#include "Client.hpp"

Client::Client() : _fd(-1) {}

Client::Client(int fd) : _fd(fd) {}

Client::~Client() {}

Client::Client(const Client &src) : _fd(src._fd), _buffer(src._buffer) {}

Client &Client :: operator=(const Client &src) 
{
    if (this != &src)
    {
        this->_fd = src._fd;
        this->_buffer = src._buffer;
    }
    return *this;
}

int Client::getFd() const {
    return _fd;
}

std::string& Client::getBuffer() {
    return _buffer;
}