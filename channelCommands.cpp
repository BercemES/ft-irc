#include "Command.hpp"
#include "Server.hpp"

#include <sstream>
#include <vector>

using std::string;
using std::vector;

static bool isValidChannelName(const string& name)
{
    if (name.size() < 2 || (name[0] != '#' && name[0] != '&'))
        return false;
    if (name.size() > 50)
        return false;
    for (size_t i = 1; i < name.size(); ++i)
    {
        if (name[i] == ' ' || name[i] == ',' || name[i] == '\a')
            return false;
    }
    return true;
}

static vector<string> splitList(const string& list)
{
    vector<string> out;
    std::stringstream ss(list);
    string item;

    while (std::getline(ss, item, ','))
        out.push_back(item);
    return out;
}

static string buildNames(const Channel& channel)
{
    string names;
    const std::map<int, Client*>& members = channel.getMembers();
    std::map<int, Client*>::const_iterator it;

    for (it = members.begin(); it != members.end(); ++it)
    {
        if (it->second == NULL)
            continue;
        if (!names.empty())
            names += " ";
        if (channel.isOperator(it->second))
            names += "@";
        names += it->second->getName(TYPE_NICK);
    }
    return names;
}

static void joinOne(Server& server, Client& client, const string& name, const string& key)
{
    if (!isValidChannelName(name))
    {
        client.sendMessage(ERR_BADCHANMASK(client.getName(TYPE_NICK), name));
        return;
    }

    Channel* chan = server.getChannel(name);
    bool created = (chan == NULL);
    if (chan != NULL)
    {
        if (chan->isMember(&client))
            return;
        if (chan->isInviteOnly() && !chan->isInvited(&client))
        {
            client.sendMessage(ERR_INVITEONLYCHAN(client.getName(TYPE_NICK), chan->getName()));
            return;
        }
        if (chan->hasKey() && !chan->checkKey(key))
        {
            client.sendMessage(ERR_BADCHANNELKEY(client.getName(TYPE_NICK), chan->getName()));
            return;
        }
        if (chan->isFull())
        {
            client.sendMessage(ERR_CHANNELISFULL(client.getName(TYPE_NICK), chan->getName()));
            return;
        }
    }
    else
        chan = &server.getOrCreateChannel(name);

    chan->addMember(&client);
    if (created)
        chan->addOperator(&client);

    string prefix = client.getFullPrefix();
    server.broadcastToChannel(*chan, prefix + " JOIN " + chan->getName() + "\r\n");

    if (chan->hasTopic())
    {
        std::ostringstream when;
        when << chan->getTopicTime();
        client.sendMessage(RPL_TOPIC(client.getName(TYPE_NICK), chan->getName(), chan->getTopic()));
        client.sendMessage(RPL_TOPICWHOTIME(client.getName(TYPE_NICK), chan->getName(),
            chan->getTopicSetter(), when.str()));
    }
    client.sendMessage(RPL_NAMREPLY(client.getName(TYPE_NICK), chan->getName(), buildNames(*chan)));
    client.sendMessage(RPL_ENDOFNAMES(client.getName(TYPE_NICK), chan->getName()));
}

static void partOne(Server& server, Client& client, const string& name, const string& reason)
{
    Channel* chan = server.getChannel(name);
    if (chan == NULL)
    {
        client.sendMessage(ERR_NOSUCHCHANNEL(client.getName(TYPE_NICK), name));
        return;
    }
    if (!chan->isMember(&client))
    {
        client.sendMessage(ERR_NOTONCHANNEL(client.getName(TYPE_NICK), chan->getName()));
        return;
    }

    string prefix = client.getFullPrefix();
    server.broadcastToChannel(*chan, prefix + " PART " + chan->getName()
        + (reason.empty() ? "" : " :" + reason) + "\r\n");

    chan->removeMember(&client);
    server.removeEmptyChannel(name);
}

void Command::handleJoin(Server& server, Client& client, const IRCMessage& msg)
{
    if (msg.params.empty() || msg.params[0].empty())
    {
        client.sendMessage(ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
        return;
    }

    vector<string> names = splitList(msg.params[0]);
    vector<string> keys;
    if (msg.params.size() >= 2)
        keys = splitList(msg.params[1]);

    for (size_t i = 0; i < names.size(); ++i)
    {
        if (names[i].empty())
            continue;
        joinOne(server, client, names[i], i < keys.size() ? keys[i] : string());
    }
}

void Command::handlePart(Server& server, Client& client, const IRCMessage& msg)
{
    if (msg.params.empty() || msg.params[0].empty())
    {
        client.sendMessage(ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
        return;
    }

    vector<string> names = splitList(msg.params[0]);
    string reason = msg.params.size() >= 2 ? msg.params[1] : string();

    for (size_t i = 0; i < names.size(); ++i)
    {
        if (names[i].empty())
            continue;
        partOne(server, client, names[i], reason);
    }
}

void Command::handleTopic(Server& server, Client& client, const IRCMessage& msg)
{
    if (msg.params.empty() || msg.params[0].empty())
    {
        client.sendMessage(ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
        return;
    }

    Channel* chan = server.getChannel(msg.params[0]);
    if (chan == NULL)
    {
        client.sendMessage(ERR_NOSUCHCHANNEL(client.getName(TYPE_NICK), msg.params[0]));
        return;
    }
    if (!chan->isMember(&client))
    {
        client.sendMessage(ERR_NOTONCHANNEL(client.getName(TYPE_NICK), chan->getName()));
        return;
    }

    if (msg.params.size() < 2)
    {
        if (!chan->hasTopic())
        {
            client.sendMessage(RPL_NOTOPIC(client.getName(TYPE_NICK), chan->getName()));
            return;
        }
        std::ostringstream when;
        when << chan->getTopicTime();
        client.sendMessage(RPL_TOPIC(client.getName(TYPE_NICK), chan->getName(), chan->getTopic()));
        client.sendMessage(RPL_TOPICWHOTIME(client.getName(TYPE_NICK), chan->getName(),
            chan->getTopicSetter(), when.str()));
        return;
    }

    if (chan->isTopicRestricted() && !chan->isOperator(&client))
    {
        client.sendMessage(ERR_CHANOPRIVSNEEDED(client.getName(TYPE_NICK), chan->getName()));
        return;
    }
    chan->setTopic(msg.params[1], client.getName(TYPE_NICK));
    server.broadcastToChannel(*chan, client.getFullPrefix()
        + " TOPIC " + chan->getName() + " :" + msg.params[1] + "\r\n");
}

void Command::handleKick(Server& server, Client& client, const IRCMessage& msg)
{
    if (msg.params.size() < 2 || msg.params[0].empty() || msg.params[1].empty())
    {
        client.sendMessage(ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
        return;
    }

    Channel* chan = server.getChannel(msg.params[0]);
    if (chan == NULL)
    {
        client.sendMessage(ERR_NOSUCHCHANNEL(client.getName(TYPE_NICK), msg.params[0]));
        return;
    }
    if (!chan->isMember(&client))
    {
        client.sendMessage(ERR_NOTONCHANNEL(client.getName(TYPE_NICK), chan->getName()));
        return;
    }
    if (!chan->isOperator(&client))
    {
        client.sendMessage(ERR_CHANOPRIVSNEEDED(client.getName(TYPE_NICK), chan->getName()));
        return;
    }

    Client* target = server.getClientByNick(msg.params[1]);
    if (target == NULL || !chan->isMember(target))
    {
        client.sendMessage(ERR_USERNOTINCHANNEL(client.getName(TYPE_NICK),
            msg.params[1], chan->getName()));
        return;
    }

    string reason = (msg.params.size() >= 3 && !msg.params[2].empty())
        ? msg.params[2] : client.getName(TYPE_NICK);

    server.broadcastToChannel(*chan, client.getFullPrefix()
        + " KICK " + chan->getName() + " " + target->getName(TYPE_NICK)
        + " :" + reason + "\r\n");

    chan->removeMember(target);
    server.removeEmptyChannel(msg.params[0]);
}

void Command::handleInvite(Server& server, Client& client, const IRCMessage& msg)
{
    if (msg.params.size() < 2 || msg.params[0].empty() || msg.params[1].empty())
    {
        client.sendMessage(ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
        return;
    }

    Channel* chan = server.getChannel(msg.params[1]);
    if (chan == NULL)
    {
        client.sendMessage(ERR_NOSUCHCHANNEL(client.getName(TYPE_NICK), msg.params[1]));
        return;
    }
    if (!chan->isMember(&client))
    {
        client.sendMessage(ERR_NOTONCHANNEL(client.getName(TYPE_NICK), chan->getName()));
        return;
    }
    if (!chan->isOperator(&client))
    {
        client.sendMessage(ERR_CHANOPRIVSNEEDED(client.getName(TYPE_NICK), chan->getName()));
        return;
    }

    Client* target = server.getClientByNick(msg.params[0]);
    if (target == NULL)
    {
        client.sendMessage(ERR_NOSUCHNICK(client.getName(TYPE_NICK), msg.params[0]));
        return;
    }
    if (chan->isMember(target))
    {
        client.sendMessage(ERR_USERONCHANNEL(client.getName(TYPE_NICK),
            target->getName(TYPE_NICK), chan->getName()));
        return;
    }

    chan->invite(target);
    client.sendMessage(RPL_INVITING(client.getName(TYPE_NICK),
        target->getName(TYPE_NICK), chan->getName()));
    server.sendToClient(*target, client.getFullPrefix()
        + " INVITE " + target->getName(TYPE_NICK) + " " + chan->getName() + "\r\n");
}
