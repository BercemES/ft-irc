#include "server.hpp"

void handleCap(server& server, Client& client, const IRCMessage& msg)
{
	
	
}

void handlePass(const server& server, Client& client, const IRCMessage& msg)
{
	if (msg.params[0].empty())
	{
		client.sendMessage(Replies::ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
		return ;
	}	
	if (client.isFullyRegistered())
	{
		client.sendMessage(Replies::ERR_ALREADYREGISTERED(client.getName(TYPE_NICK)));
		return ;
	}
	if (msg.params[0] != server.getPassword())
	{
		client.sendMessage(Replies::ERR_PASSWDMISMATCH(client.getName(TYPE_NICK)));
		//sunucuyu kapatacaksak o fonksiyon gelecek.
	}
	client.setRegFlag(FLAG_PASS, true);
}

static bool	checkNickname( const string nick)
{
	string	acceptableSymbols = "[]\\`_^{|}";

	if (std::isdigit(nick[0]))
		return (false);
	if (nick[0] == ':' || nick[0] == '#'  || nick[0] == '&')
		return (false);
	for (size_t i = 0; i < nick.size(); i++)
	{
		if (std::isspace(nick[i]))
            return (false);
		if (!std::isalpha(nick[i]) && !std::isdigit(nick[i])
			&& acceptableSymbols.find(nick[i]) == string::npos)
			return (false);
	}
	return (true);
}

static bool	nicknameInUse(const server& server, const std::string& nick)
{
	std::map<int, Client>					clients;
	std::map<int, Client>::const_iterator	it;

	clients = server.getClients();
	for (it = clients.begin(); it != clients.end(); it++)
	{
		if (it->second.getName(TYPE_NICK) == nick)
			return (true);
	}
	return (false);
}

void handleNick(server& server, Client& client, const IRCMessage& msg)
{
	if (msg.params[0].empty())
	{
		client.sendMessage(Replies::ERR_NONICKNAMEGIVEN(client.getName(TYPE_NICK)));
		return ;
	}
	if (!checkNickname(msg.params[0]))
	{
		client.sendMessage(Replies::ERR_ERRONEUSNICKNAME(client.getName(TYPE_NICK), msg.params[0]));
		return ;
	}
	if (client.getName(TYPE_NICK) != "*")
	{
		if (nicknameInUse(server, msg.params[0]))
		{
			client.sendMessage(Replies::ERR_NICKNAMEINUSE(client.getName(TYPE_NICK), msg.params[0]));
			return ;
		}
	}
	client.setName(TYPE_NICK, msg.params[0]);

	/*
	NICK command was successful, and to inform other clients about the change of nickname. 
	In these cases, the <source> of the message will be the old nickname 
	[ [ "!" user ] "@" host ] of the user who is changing their nickname.
	*/
}

void handleUser(server& server, Client& client, const IRCMessage& msg)
{
	
}



