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

    Client* findClientByNick(Server& server, const std::string& nickname)
    {
        std::map<int, Client>& clients = server.getClients();
        std::map<int, Client>::iterator it;

        for (it = clients.begin(); it != clients.end(); ++it)
        {
            if (Client::ircToLower(it->second.getName(TYPE_NICK)) == nickname)
                return &it->second;
        }
        return NULL;
    }

    void sendToUser(Server& server, Client& sender, const std::string& target,
        const std::string& text, bool isNotice)
    {
        Client* recipient = findClientByNick(server, target);
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
            {
                // Kanal kaydi henuz Server'a tasinmadi. JOIN entegrasyonuna
                // kadar kanal hedefleri mevcut degilmis gibi davranir.
                if (!isNotice)
                    client.sendMessage(ERR_NOSUCHCHANNEL(client.getName(TYPE_NICK), *it));
            }
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

void Command::handlePrivmsg(Server& server, Client& client, const IRCMessage& msg)
{
    handleMessage(server, client, msg, false);
}

void Command::handleNotice(Server& server, Client& client, const IRCMessage& msg)
{
    handleMessage(server, client, msg, true);
}
