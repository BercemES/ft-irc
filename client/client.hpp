#ifndef CLIENT_HPP
# define CLIENT_HPP


#include <iostream>
#include <vector>

using std::string;
using std::cout;
using std::endl;

struct IRCMessage {
	string prefix;
	string command;
	std::vector<std::string> params;
};

class Client
{
private:
	int		_fd;
	string	_buffer;
	string	_nickname;
	string	_username;
	string	_realname;
	bool	_passFlag;
	bool	_nickFlag;
	bool	_userFlag;

	IRCMessage		parseIRC(const std::string& line);
	static string	nextParam(const std::string& line, size_t& pos);
	static string	ircToLower(string str);
public:
	Client(int fd);
	~Client();

	void	appendToBuffer(const std::string& message);
	bool	isFullyRegistered() const;
};


#endif

