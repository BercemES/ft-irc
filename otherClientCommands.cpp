#include "command.hpp"
#include "server.hpp"

void	handleMotd(server& server, Client& client, const IRCMessage& msg)
{
	if (!msg.params.empty() && msg.params[0] != "ircserv")
	{
		client.sendMessage(ERR_NOSUCHSERVER(client.getName(TYPE_NICK), msg.params[0]));
		return ;
	}
	client.sendMessage(RPL_MOTDSTART(client.getName(TYPE_NICK)));
	client.sendMessage(RPL_MOTD(client.getName(TYPE_NICK), "Welcome to ft_irc"));
	client.sendMessage(RPL_MOTD(client.getName(TYPE_NICK), "Developed by Team ...."));
	//BURAYA team adı gelecek
	client.sendMessage(RPL_MOTD(client.getName(TYPE_NICK), "Enjoy your stay."));
	client.sendMessage(RPL_ENDOFMOTD(client.getName(TYPE_NICK)));
}


std::vector<string>	splitTargets(Client& client, const IRCMessage& msg)
{
	std::vector<string>	targets;
	std::stringstream	ss;
	string				target;

	ss << msg.params[0];
	while (std::getline(ss, target, ','))
	{
		if (target.empty())
			continue ;
		target = client.ircToLower(target);
		std::vector<string>::iterator it = std::find(targets.begin(), targets.end(), target);
		if (it == targets.end())
			targets.push_back(target);
	}
	return (targets);
}

void	sendToChannel(server& server, Client& client, const IRCMessage& msg, std::vector<string>::iterator it)
{
	map<int, Channel> chan = server.getChannels();
	map<int, Channel>::iterator itChan = chan.begin();
	for (itChan; itChan != chan.end(); itChan++)
	{
		if (*it == itChan->second.getChannelName())
		{
			if (!isBanned(client.getName(TYPE_NICK)))//mode flagi olarak +n no external msg kullanırsak || olarak o da eklenecek
			{
				string	text;
				text = ":" + itChan->second.getChannelName() + "!" + client.getName(TYPE_USER) +
				"@" + client.getHost() + " PRIVMSG " + *it + " :" + msg.params.back() + "\r\n";
				for (i = 0; i < chan.getUser(); i++) //channeldaki tüm kullanıcılara mesajı gönder döngüsü
					client.sendMessage(text);
				return ;
			}
			else
			{
				client.sendMessage(ERR_CANNOTSENDTOCHAN(client.getName(TYPE_NICK), msg.params[0]));
				return ;
			}
		}
	}
	client.sendMessage(ERR_NOSUCHNICK(client.getName(TYPE_NICK), msg.params[0]));
}

void	sendToUser(server& server, Client& client, const IRCMessage& msg, std::vector<string>::iterator it)
{
	map<int, Client> cli = server.getClients();
	map<int, Client>::iterator itCli = cli.begin();
	for (itCli; itCli != cli.end(); itCli++)
	{
		if (*it == itCli->second.getName(TYPE_NICK))
		{
			string	text;
			text = ":" + client.getName(TYPE_NICK) + "!" + client.getName(TYPE_USER) +
			"@" + client.getHost() + " PRIVMSG " + *it + " :" + msg.params.back() + "\r\n";
			client.sendMessage(text);
			return ;
		}
	}
	client.sendMessage(ERR_NOSUCHNICK(client.getName(TYPE_NICK), msg.params[0]));
}

void	handlePrivmsg(server& server, Client& client, const IRCMessage& msg)
{
	if ( msg.params[0].empty())
	{
		client.sendMessage(ERR_NORECIPIENT(client.getName(TYPE_NICK), msg.command));
		return ;
	}
	if (msg.params[1].empty())
	{
		client.sendMessage(ERR_NOTEXTTOSEND(client.getName(TYPE_NICK)));
		return ;
	}
	std::vector<string>	targets = splitTargets(client, msg);
	std::vector<string>::iterator it;
	for (it = targets.begin(); it < targets.end(); it++)
	{
		if ((*it)[0] == '#' || (*it)[0] == '&')
			sendToChannel(server, client, msg, it);
		else
			sendToUser(server, client, msg, it);
	}
}

void	handleNotice(server& server, Client& client, const IRCMessage& msg)
{

}

