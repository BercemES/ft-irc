#include "command.hpp"
#include "Server.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

namespace
{
	std::vector<std::string> splitTargets(const std::string& targetList)
	{
		std::vector<std::string> targets;
		std::stringstream stream(targetList);
		std::string target;

		while (std::getline(stream, target, ','))
		{
			if (target.empty())
				continue;
			target = Client::ircToLower(target);
			if (std::find(targets.begin(), targets.end(), target) == targets.end())
				targets.push_back(target);
		}
		return targets;
	}

    void sendToChannel(Server& server, Client& sender, const std::string& target,
        const std::string& text, bool isNotice)
    {
        Channel* channel = server.getChannel(target);
        if (channel == NULL)
        {
            if (!isNotice)
                sender.sendMessage(ERR_NOSUCHCHANNEL(sender.getName(TYPE_NICK), target));
            return;
        }
        if (!channel->isMember(&sender))
        {
            if (!isNotice)
                sender.sendMessage(ERR_CANNOTSENDTOCHAN(sender.getName(TYPE_NICK),
                    channel->getName()));
            return;
        }
        server.broadcastToChannel(*channel, ":" + sender.getName(TYPE_NICK) + "!"
            + sender.getName(TYPE_USER) + "@" + sender.getHost()
            + (isNotice ? " NOTICE " : " PRIVMSG ") + channel->getName()
            + " :" + text + "\r\n", &sender);
    }

    void sendToUser(Server& server, Client& sender, const std::string& target,
        const std::string& text, bool isNotice)
    {
        Client* recipient = server.getClientByNick(target);
        if (recipient == NULL)
        {
            if (!isNotice)
                sender.sendMessage(ERR_NOSUCHNICK(sender.getName(TYPE_NICK), target));
            return;
        }
        server.sendToClient(*recipient, ":" + sender.getName(TYPE_NICK) + "!"
            + sender.getName(TYPE_USER) + "@" + sender.getHost()
            + (isNotice ? " NOTICE " : " PRIVMSG ") +  recipient->getName(TYPE_NICK) + " :" + text + "\r\n");
    }

	void handleMessage(Server& server, Client& client, const IRCMessage& msg,
		bool isNotice)
	{
		if (msg.params.empty() || msg.params[0].empty())
		{
			if (!isNotice)
				client.sendMessage(ERR_NORECIPIENT(client.getName(TYPE_NICK), msg.command));
			return;
		}
		if (msg.params.size() < 2 || msg.params[1].empty())
		{
			if (!isNotice)
				client.sendMessage(ERR_NOTEXTTOSEND(client.getName(TYPE_NICK)));
			return;
		}

        std::vector<std::string> targets = splitTargets(msg.params[0]);
        if (targets.empty())
        {
            if (!isNotice)
                client.sendMessage(ERR_NORECIPIENT(client.getName(TYPE_NICK), msg.command));
            return;
        }
        for (std::vector<std::string>::const_iterator it = targets.begin();
            it != targets.end(); ++it)
        {
            if ((*it)[0] == '#' || (*it)[0] == '&')
                sendToChannel(server, client, *it, msg.params[1], isNotice);
            else
                sendToUser(server, client, *it, msg.params[1], isNotice);
        }
    }
}

void Command::handleMotd(Server&, Client& client, const IRCMessage& msg)
{
	if (!msg.params.empty() && msg.params[0] != "ircserv")
	{
		client.sendMessage(ERR_NOSUCHSERVER(client.getName(TYPE_NICK), msg.params[0]));
		return;
	}
	std::ifstream file("motd.txt");
	if (!file.is_open())
	{
		client.sendMessage(ERR_NOMOTD(client.getName(TYPE_NICK)));
		return;
	}
	client.sendMessage(RPL_MOTDSTART(client.getName(TYPE_NICK)));
	std::string  text;
	while (getline(file, text))
		client.sendMessage(RPL_MOTD(client.getName(TYPE_NICK), text));
	client.sendMessage(RPL_ENDOFMOTD(client.getName(TYPE_NICK)));
	file.close();
}

void Command::handlePrivmsg(Server& server, Client& client, const IRCMessage& msg)
{
	handleMessage(server, client, msg, false);
}

void Command::handleNotice(Server& server, Client& client, const IRCMessage& msg)
{
	handleMessage(server, client, msg, true);
}
