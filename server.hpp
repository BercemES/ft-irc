#ifndef SERVER_HPP
# define SERVER_HPP

# include <iostream>
# include <cctype>
# include <vector>
# include <map>
# include <sstream>
#include <algorithm>

# include "client.hpp"
# include "command.hpp"
# include "replies.hpp"
# include "channel.hpp"

using std::string;
using std::vector;
using std::map;

class Client; 
struct IRCMessage;

typedef void (*CmdFunc)(server&, Client&, const IRCMessage&);

class server
{
private:
	int							_serverFd;
	map<int, Client>			_clients;
	map<int, Channel>			_channels;

	const string	_password;
	map<string, string> _RPL_ISUPPORT;

	std::map<std::string, CmdFunc> _commands;

	void						initCommands();
	void						initIsupport();

	void executeCommand(Client& client, const IRCMessage& msg);
	//COMMAND UTILS
	bool checkNickname(const std::string nick);
    bool nicknameInUse(const server& serv, const std::string& nick);
public:
	server(/* args */);
	~server();

	void					handleCommand(Client& client, IRCMessage& msg);
	const string			getPassword() const;
	const map<int, Client>&	getClients() const;
	const string			getIsupport(const string& key) const;
	bool					checkReg(Client& client) const;    
};


#endif