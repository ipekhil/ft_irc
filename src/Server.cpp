#include "Server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <sstream>
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
	else
		std::cout << "RECV";

    //gelen veriyi buffera ekler

	std::string& buf = _clients[client_fd].getBuffer();
	buf.append(temp_buf, bytes_read);
	//npos no posiiton demektir.npos döndünce döngüden çık dmektir yani bulunamadığında
	size_t pos = 0;
	while ((pos = buf.find('\n')) != std::string::npos)
	{
		std::string line = buf.substr(0, pos);
		buf.erase(0, pos + 1);
		if (!line.empty() && line[line.size() - 1] == '\r')
    		line.erase(line.size() - 1);
		handleCommand(client_fd, line);
	}
	
}

void Server::handleCommand(int client_fd, const std::string& line) {
    std::istringstream iss(line);
    std::string command;
    iss >> command;
	//iss 

	//en başta boşluk varsa atlar ve mesela PASSden sonrak ilk boşluğa kadar oaln kısmı command'a atar.
    if (command.empty())
        return;

	//geri kalan her sözcüğü bir argüman olarak alır ve arka arkaya ekler
    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg)
	{
		//: olan komutları işler
		if (arg[0] == ':')
		{
			std::string rest;
			//:'dan sonrasını komple restin içine atar
			std::getline(iss, rest);
			args.push_back(arg.substr(1) + rest);
			std::cout << arg.substr(1) + rest << std::endl;
			break;
		}
        args.push_back(arg);
	}

	//cilent_fd ile clint takma adına eşliyoruz & olma sebebi kopysasının oluşturlmamasının istenmesi
    Client& client = _clients[client_fd];

	//sunucuya şifresi ile giriş
    if (command == "PASS") {
        if (args.empty()) {
			sendReply(client_fd, ":ircserv 461 * PASS :Not enough parameters");
			return;
        }
        if (client.getHasPassword() || client.getIsRegistered()) {
            sendReply(client_fd, ":ircserv 462 * :You may not reregister");
            return;
        }
        if (args[0] == _password) {
            client.setHasPassword(true);
            std::cout << "[LOG] Client fd " << client_fd << " authenticated." << std::endl;
        } else {
            sendReply(client_fd, ":ircserv 464 * :Password incorrect");
        }
    }
	else if (command == "NICK")
	{
		if (!client.getHasPassword()) {
			sendReply(client_fd, ":ircserv 451 * :You have not registered");
			return;
		}
		if (args.empty()) {
			sendReply(client_fd, ":ircserv 461 * NICK :Not enough parameters");
			return;
		}
		std::map<int, Client>::iterator it = _clients.begin();
		while (it != _clients.end())
		{
			if (it->first != client_fd && it->second.getNickname() == args[0]) {
                sendReply(client_fd, ":ircserv 433 * " + args[0] + " :Nickname is already in use");
                return;
            }
			++it;
		}
		client.setNickname(args[0]);
		std::cout << "[LOG] Client fd " << client_fd << " set nickname to: " << args[0] << std::endl;
	}
	else if (command == "USER")
    {
        if (!client.getHasPassword()) {
            sendReply(client_fd, ":ircserv 451 * :You have not registered");
            return;
        }
        if (client.getIsRegistered()) {
            sendReply(client_fd, ":ircserv 462 * :You may not reregister");
            return;
        }
        if (args.size() < 4) {
            sendReply(client_fd, ":ircserv 461 * USER :Not enough parameters");
            return;
        }
        client.setUsername(args[0]);
    }

    if (!client.getIsRegistered() && client.getHasPassword()
        && !client.getNickname().empty() && !client.getUsername().empty()) {
        client.setIsRegistered(true);
        sendReply(client_fd, ":ircserv 001 " + client.getNickname() + " :Welcome to the ft_irc Network, " + client.getNickname());
        std::cout << "[LOG] Client fd " << client_fd << " registered as " << client.getNickname() << std::endl;
    }
	
	
}

//sunucudan istemciye mesaj gönderirken her zaman \r\n ekliyoruz mesajın sonuna bu şekilde daha güvenli oluyprmuş
void Server::sendReply(int client_fd, const std::string& message) {
    std::string full_msg = message + "\r\n";
    if (send(client_fd, full_msg.c_str(), full_msg.size(), 0) < 0)
        std::cerr << "[ERROR] Failed to send message to fd: " << client_fd << std::endl;
}