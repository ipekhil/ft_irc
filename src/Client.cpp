#include "Client.hpp"

Client::Client() : _fd(-1) {}

Client::Client(int fd) : _fd(fd), _hasPassword(false), _isRegistered(false) {}

Client::~Client() {}

Client::Client(const Client &src) : _fd(src._fd), _buffer(src._buffer), 
_hasPassword(false), _isRegistered(false),
_nickname(src._nickname), _username(src._username) {}

Client &Client :: operator=(const Client &src) 
{
    if (this != &src)
    {
        this->_fd = src._fd;
        this->_buffer = src._buffer;
		this->_hasPassword = src._hasPassword;
		this->_isRegistered = src._isRegistered;
		this->_nickname = src._nickname;
		this->_username = src._username;
    }
    return *this;
}

int Client::getFd() const {
    return _fd;
}

std::string& Client::getBuffer() {
    return _buffer;
}

bool Client::getHasPassword() const {
    return _hasPassword;
}

void Client::setHasPassword(bool hasPassword) {
    _hasPassword = hasPassword;
}

bool Client::getIsRegistered() const {
    return _isRegistered;
}

void Client::setIsRegistered(bool isRegistered) {
    _isRegistered = isRegistered;
}

const std::string& Client::getNickname() const {
    return _nickname;
}

void Client::setNickname(const std::string& nickname) {
    _nickname = nickname;
}

const std::string& Client::getUsername() const {
    return _username;
}

void Client::setUsername(const std::string& username) {
    _username = username;
}