#ifndef SERVER_HPP
# define SERVER_HPP

#include <iostream>
#include <vector>
#include <map>

#include "client.hpp"
#include "Acommand.hpp"
#include "registration.hpp"

class server
{
private:
	int								_serverFd;
	std::map<int, Client>			_clients;
	//_channels;
	std::map<std::string, Acommand*> _commands;

	void initCommands();

public:
	server(/* args */);
	~server();

	void	handleCommand(Client& client, IRCMessage& msg);
};


#endif