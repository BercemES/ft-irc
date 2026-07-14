#include "command.hpp"
#include "server.hpp"

void Command::handlePass(server& server, Client& client, const IRCMessage& msg)
{
	if (msg.params[0].empty())
	{
		client.sendMessage(ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
		return ;
	}	
	if (client.isFullyRegistered())
	{
		client.sendMessage(ERR_ALREADYREGISTERED(client.getName(TYPE_NICK)));
		return ;
	}
	if (msg.params[0] != server.getPassword())
	{
		client.sendMessage(ERR_PASSWDMISMATCH(client.getName(TYPE_NICK)));
		//sunucuyu kapatacaksak o fonksiyon gelecek.
	}
	client.setRegFlag(FLAG_PASS, true);
}

bool	checkNickname( const string nick)
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

bool	nicknameInUse(const server& server, const std::string& nick)
{
	std::map<int, Client>					clients = server.getClients();;
	std::map<int, Client>::const_iterator	it;

	for (it = clients.begin(); it != clients.end(); it++)
	{
		if (Client::ircToLower(it->second.getName(TYPE_NICK)) == Client::ircToLower((nick)))
			return (true);
	}
	return (false);
}

void Command::handleNick(server& server, Client& client, const IRCMessage& msg)
{
	if (!client.getRegFlag(FLAG_PASS))
		return ;
	if (msg.params[0].empty())
	{
		client.sendMessage(ERR_NONICKNAMEGIVEN(client.getName(TYPE_NICK)));
		return ;
	}
	if (!checkNickname(msg.params[0]))
	{
		client.sendMessage(ERR_ERRONEUSNICKNAME(client.getName(TYPE_NICK), msg.params[0]));
		return ;
	}
	if (nicknameInUse(server, msg.params[0]))
	{
		client.sendMessage(ERR_NICKNAMEINUSE(client.getName(TYPE_NICK), msg.params[0]));
		return ;
	}
	if (!client.isFullyRegistered())
	{
		client.setName(TYPE_NICK, msg.params[0]);
		client.setRegFlag(FLAG_NICK, true);
		server.checkReg(client);
	}
	else
	{
		map<int, Channel> chan = server.getChannels();
		string	oldNick = client.getName(TYPE_NICK);
		client.setName(TYPE_NICK, msg.params[0]);
		string	text = ":" + oldNick + "!" + client.getName(TYPE_USER) + "@" + client.getHost() + " "
			+ msg.command + " " + client.getName(TYPE_NICK) + "\r\n";
		for (i = 0; i < chan.getUser(); i++) //channeldaki tüm kullanıcılara mesajı gönder döngüsü
			client.sendMessage(text);
	}
	
}

void Command::handleUser(server& server, Client& client, const IRCMessage& msg)
{
	if (!client.getRegFlag(FLAG_PASS))
		return ;
	if (client.isFullyRegistered())
	{
		client.sendMessage(ERR_ALREADYREGISTERED(client.getName(TYPE_NICK)));
		return ;
	}
	if (msg.params.empty() || msg.params[0].length() < 1 || msg.params.size() < 4)
	{
		client.sendMessage(ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
		return ;
	}
	string userlen_str = server.getIsupport("USERLEN");
	std::stringstream ss(userlen_str);
	int max_userlen = 0;
	ss >> max_userlen;
	string username = msg.params[0];
	if (msg.params[0].length() > max_userlen)
		username.erase(max_userlen);
	client.setName(TYPE_USER, ("~" + username));
	client.setName(TYPE_REAL, msg.params.back());
	client.setRegFlag(FLAG_USER, true);
	server.checkReg(client);
}
