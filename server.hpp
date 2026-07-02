#ifndef SERVER_HPP
# define SERVER_HPP

#include <iostream>
#include <cctype>
#include <vector>
#include <map>

#include "client.hpp"
#include "replies.hpp"

class server
{
private:
	int								_serverFd;
	std::map<int, Client>			_clients;
	//_channels;

	string	_password;

	void initCommands();

public:
	server(/* args */);
	~server();

	void						handleCommand(Client& client, IRCMessage& msg);
	const string				getPassword() const;
	const std::map<int, Client>	getClients() const;
};


#endif