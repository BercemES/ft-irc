#ifndef SERVER_HPP
# define SERVER_HPP

#include <iostream>
#include <cctype>
#include <vector>
#include <map>
#include <sstream>

#include "client.hpp"
#include "replies.hpp"

class server
{
private:
	int								_serverFd;
	std::map<int, Client>			_clients;
	//_channels;

	const string	_password;
	std::map<std::string, std::string> _RPL_ISUPPORT;

	void						initCommands();
	void						initIsupport();

public:
	server(/* args */);
	~server();

	void						handleCommand(Client& client, IRCMessage& msg);
	const string				getPassword() const;
	const std::map<int, Client>	getClients() const;
	const string				getIsupport(const std::string& key) const;
};


#endif