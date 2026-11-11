#include "Server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>

Server::Server(int port, const std::string& password) 
    : _port(port), _password(password), _server_fd(-1) {}

Server::~Server() {
    if (_server_fd != -1)
        close(_server_fd);
}

void Server::init() {
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd < 0)
        throw std::runtime_error("Failed to create server socket");

    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("Failed to set SO_REUSEADDR");

    if (fcntl(_server_fd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("Failed to set non-blocking");

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(_port);

    if (bind(_server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        throw std::runtime_error("Failed to bind socket");

    if (listen(_server_fd, SOMAXCONN) < 0)
        throw std::runtime_error("Failed to listen");

    std::cout << "[SUCCESS] Server initialized and listening on port " << _port << std::endl;
}

void Server::start() {
    std::cout << "[INFO] Test mode: Server will stay alive for 20 seconds..." << std::endl;
    sleep(20); // Test edebilmemiz için programı 20 saniye açık tutuyoruz
}