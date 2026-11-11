#include "Server.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cctype>
#include <csignal>

static bool isValidPort(const std::string& port_str) {
    if (port_str.empty())
        return false;

    for (size_t i = 0; i < port_str.length(); ++i) {
        if (!std::isdigit(port_str[i]))
            return false;
    }

    int port = std::atoi(port_str.c_str());
    if (port < 1024 || port > 65535)
        return false;

    return true;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    // Bu sunucu çalışırken SIGPIPE sinyali meydana gelirse programımı kapatıp çökertme, bu sinyali yoksay
    signal(SIGPIPE, SIG_IGN);

    if (!isValidPort(argv[1])) {
        std::cerr << "Error: Invalid port. Port must contain digits only (1024-65535)." << std::endl;
        return 1;
    }

    std::string password = argv[2];
    if (password.empty()) {
        std::cerr << "Error: Password cannot be empty." << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);

    try {
        Server server(port, password);
        server.init();
        server.start();
    } catch (const std::exception &e) {
        std::cerr << "Server Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}