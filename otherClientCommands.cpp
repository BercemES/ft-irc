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
            + (isNotice ? " NOTICE " : " PRIVMSG ") + target + " :" + text + "\r\n");
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
    client.sendMessage(RPL_MOTDSTART(client.getName(TYPE_NICK)));
    client.sendMessage(RPL_MOTD(client.getName(TYPE_NICK), "Welcome to ft_irc"));
    client.sendMessage(RPL_MOTD(client.getName(TYPE_NICK), "Enjoy your stay."));
    client.sendMessage(RPL_ENDOFMOTD(client.getName(TYPE_NICK)));
}

// WHO <#kanal> | WHO <nick>
// irssi JOIN sonrasinda otomatik WHO gonderir; bunu 421 yerine dogru
// cevaplamak icin minimal destek. Mask/away/ircop semantigi ve full Modern
// IRC WHO davranisi (bkz. modern.ircdocs.horse) kapsam disidir.
void Command::handleWho(Server& server, Client& client, const IRCMessage& msg)
{
    if (msg.params.empty() || msg.params[0].empty())
    {
        client.sendMessage(RPL_ENDOFWHO(client.getName(TYPE_NICK), "*"));
        return;
    }

    const std::string& target = msg.params[0];
    if (target[0] == '#' || target[0] == '&')
    {
        Channel* chan = server.getChannel(target);
        if (chan != NULL)
        {
            const std::map<int, Client*>& members = chan->getMembers();
            for (std::map<int, Client*>::const_iterator it = members.begin();
                it != members.end(); ++it)
            {
                if (it->second == NULL)
                    continue;
                std::string flags = chan->isOperator(it->second) ? "H@" : "H";
                client.sendMessage(RPL_WHOREPLY(client.getName(TYPE_NICK), chan->getName(),
                    it->second->getName(TYPE_USER), it->second->getHost(),
                    it->second->getName(TYPE_NICK), flags, it->second->getName(TYPE_REAL)));
            }
        }
        client.sendMessage(RPL_ENDOFWHO(client.getName(TYPE_NICK), target));
        return;
    }

    Client* targetClient = server.getClientByNick(target);
    if (targetClient != NULL)
    {
        client.sendMessage(RPL_WHOREPLY(client.getName(TYPE_NICK), "*",
            targetClient->getName(TYPE_USER), targetClient->getHost(),
            targetClient->getName(TYPE_NICK), "H", targetClient->getName(TYPE_REAL)));
    }
    client.sendMessage(RPL_ENDOFWHO(client.getName(TYPE_NICK), target));
}

void Command::handlePrivmsg(Server& server, Client& client, const IRCMessage& msg)
{
    handleMessage(server, client, msg, false);
}

void Command::handleNotice(Server& server, Client& client, const IRCMessage& msg)
{
    handleMessage(server, client, msg, true);
}
