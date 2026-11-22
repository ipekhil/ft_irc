#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
private:
    int _fd;
    std::string _buffer;
	bool        _hasPassword;
    bool        _isRegistered;
    std::string _nickname;
    std::string _username;

public:
    Client();
    Client(int fd);
    Client(const Client &src);
    Client &operator=(const Client &src);
    ~Client();

    int getFd() const;
    std::string& getBuffer();

	bool getHasPassword() const;
	void setHasPassword(bool hasPassword);
	bool getIsRegistered() const;
	void setIsRegistered(bool isRegistered);
	const std::string& getNickname() const;
	void setNickname(const std::string& nickname);
	const std::string& getUsername() const;
	void setUsername(const std::string& username);

};

#endif