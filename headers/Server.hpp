#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include <string>
#include <vector>
#include <map>
#include <poll.h>

class Server {
private:
    int _port; //dinlenecek port no
    std::string _password; //şifresi
    int _server_fd; //yeni bağlantıları karşılayan soket no
    std::vector<struct pollfd> _fds; //aktif sokt listesi
    std::map<int, Client> _clients; //soket no dan clienta ulaşmayı sağlar
    
    //gğvelik sebebyile bellekte kopyalanmasın diye consları privte yazıyoruz böylece ort kuralını da bozmuyor
    Server();
    Server(const Server &src);
    Server &operator=(const Server &src);
    
    void setupSocket();
    void acceptNewClient();
    void handleClientData(size_t index);
	void handleCommand(int client_fd, const std::string& line);
	void sendReply(int client_fd, const std::string& message);
    
public:
    Server(int port, const std::string& password);
    ~Server();

    void start();
};

#endif