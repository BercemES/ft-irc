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

void	handlePrivmsg(server& server, Client& client, const IRCMessage& msg)
{
	if (msg.params.size() < 2 || msg.params[0].empty() || msg.params[1].empty())
	{
		client.sendMessage(ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
		return ;
	}
	std::vector<string>	targets = splitTargets(client, msg);
	std::vector<string>::iterator it;

	for (it = targets.begin(); it < targets.end(); it++)
	{
		if ((*it)[0] == '#' || (*it)[0] == '&')
		{
			/*if()Böyle bir channel var mı?
			{
				if(varsa bu channela bu kullanıcı kayıtlı mı?)
					istemci bu kanaldan yasaklanmış (banned) durumdaysa ve 
					herhangi bir yasak istisnası (ban exception) kapsamında değilse, 
					mesaj teslim edilmez ve komut sessizce başarısız olur
					(istemciye herhangi bir hata gönderilmeyebilir). 
					Ayrıca Moderate mesajı kabul etmeyebilir ya da değiştirebilir.!!
				else 
					durumunda ERR_CANNOTSENDTOCHAN (404)
			}
			else
				ERR_NOSUCHCHANNEL
			*** eğer STATUSMSG adlı RPL_ISUPPORT ta başka kanal türleri eklemezsek 
			kullanıcı bunlarla başlayan bir mesaj göndermemeli ama hata göndermiyoruz. 
			*/
		}
		map<int, Client> cli = server.getClients();
		map<int, Client>::iterator itCli = cli.begin();
		bool	findFlag = false;
		for (itCli; itCli != cli.end(); itCli++)
		{
			if (*it == itCli->second.getName(TYPE_NICK))
			{
				client.sendMessage();
				findFlag = true;
				break ;
			}
			
		}
		
		
		
	}
	
	
	
	
}

void	handleNotice(server& server, Client& client, const IRCMessage& msg)
{

}

