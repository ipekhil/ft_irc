#include "Server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iostream>

Server::Server(int port, const std::string& password) : _port(port), _password(password), _server_fd(-1) 
{
    setupSocket();
}

Server::~Server() {
    for (size_t i = 0; i < _fds.size(); ++i) {
        if (_fds[i].fd >= 0)
            close(_fds[i].fd);
    }
}

void Server::setupSocket() {
    //IPv4 TCP socket
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd < 0)
        throw std::runtime_error("Failed to create server socket");

    //sunucuyu kapatıp hemen yedniden çalıştırdığında kitlenmesini önler, REYSEADDR ile sağlanır,TIME_WAİT negellenir
    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(_server_fd);
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }

    //non blocking!!!fork yasak, sunucuların aynı anda çalışabilmesi için non blocking flagi kullanırız
    if (fcntl(_server_fd, F_SETFL, O_NONBLOCK) < 0) {
        close(_server_fd);
        throw std::runtime_error("Failed to set server socket to non-blocking");
    }

    //serverı yapılandırır
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));//oluşturulan adres sıfırlanır
    server_addr.sin_family = AF_INET;//ipv+
    server_addr.sin_addr.s_addr = INADDR_ANY;//0.0.0.0, tüm ağlardan gelen
    server_addr.sin_port = htons(_port);//işlemci little endian bayt sırasındadır, ağlar big endian, aralarındaki farkı eşitlemek içn htons kullanılır

    //soketi serverın portuna ve adresine bağlar
    if (bind(_server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(_server_fd);
        throw std::runtime_error("Failed to bind socket to port");
    }

    // tcp 3 way handshake 1.syn 2.syn-ack 3ack 
    // listen çağrıldığında istemci 2 bekleme kuyruğu açar 1.syn queue : syn olmuş ama 3. ack olmamış, yarım bağlantı kuyruğu
    // 1.accept queue : 3way handshake bitmiş, tcp bağlantısı fiziksel olarak kurulmuş, SOMAXCONN bu kuyruğun uzunluğunu belirler.
    // accept bu kuyruktan çeker.
    if (listen(_server_fd, SOMAXCONN) < 0) {
        close(_server_fd);
        throw std::runtime_error("Failed to listen on socket");
    }

    // ana sunucuyu poll fonksiyonunu izleyeceği soketlerin en bşına ekliyoruz
    struct pollfd server_pfd;
    server_pfd.fd = _server_fd;
    server_pfd.events = POLLIN;
    server_pfd.revents = 0;
    _fds.push_back(server_pfd);

    std::cout << "[SUCCESS] Server initialized on port " << _port << std::endl;
}

void Server::start() {
    std::cout << "[INFO] Entering main poll() loop..." << std::endl;

    while (true) {
        int poll_count = poll(&_fds[0], _fds.size(), -1);//listedeki tğm soketleri süresi izle. -1 süresz beklemek demektir
        if (poll_count < 0) {
            std::cerr << "[ERROR] poll() failed" << std::endl;
            break;
        }

        for (size_t i = 0; i < _fds.size(); ++i) {//tüm soketleri tarıyor haerekti tespiit etmeye çalışıyor
            if (_fds[i].revents == 0)
                continue;
            if (i == 0) {
                acceptNewClient();
            } 
            else {
                handleClientData(i);
                break;//kullanıcı bağlantısını keserse indisler kayar bu sebeple handle ettikten sonra döngüdeden çıkmasını sağlıyoruz
            }
        }
    }
}

void Server::acceptNewClient() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    //accept queue daki listeden almaya başlar ve istemci soketine atar
    int client_fd = accept(_server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        std::cerr << "[ERROR] Failed to accept new client" << std::endl;
        return;
    }

    //yeni doğan her soket otomatik olarak blocking doğer o sebeple istemci soketini non blocking yapmamız gerekir
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "[ERROR] Failed to set client socket to non-blocking" << std::endl;
        close(client_fd);
        return;
    }

    //poll fonksiyonunun izleme listesine ekler
    struct pollfd client_pfd;
    client_pfd.fd = client_fd;
    client_pfd.events = POLLIN;
    client_pfd.revents = 0;
    //aktif soket listesinin sonuna ekler
    _fds.push_back(client_pfd);

    //client_fdyi Clients nesnesi ile eşler
    _clients.insert(std::make_pair(client_fd, Client(client_fd)));

    std::cout << "[LOG] New client connected! Assigned fd: " << client_fd << std::endl;
}

void Server::handleClientData(size_t index) {
    int client_fd = _fds[index].fd;
    char temp_buf[1024];
    std::memset(temp_buf, 0, sizeof(temp_buf));

    //read gibidir ama ağ okuması yaptığı anlaşılır
    ssize_t bytes_read = recv(client_fd, temp_buf, sizeof(temp_buf) - 1, 0);

    if (bytes_read <= 0) {
        std::cout << "[LOG] Client disconnected! Closed fd: " << client_fd << std::endl;
        close(client_fd);
        _clients.erase(client_fd);
        _fds.erase(_fds.begin() + index);
        return;
    }

    //gelen veriyi buffera ekler
    _clients[client_fd].getBuffer().append(temp_buf, bytes_read);
    std::cout << "[MSG from fd " << client_fd << "]: " << temp_buf << std::flush;
}

