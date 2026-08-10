#include "Command.hpp"
#include "Server.hpp"

#include <cstdlib>

using std::string;

static void appendApplied(string& applied, char& lastSign, bool adding, char mode)
{
    char sign = adding ? '+' : '-';

    if (lastSign != sign)
    {
        applied += sign;
        lastSign = sign;
    }
    applied += mode;
}

static bool isChannelTarget(const std::string& target)
{
    return (!target.empty() && (target[0] == '#' || target[0] == '&'));
}

static void handleUserModeQuery(Client& client, const std::string& target)
{
    if (Client::ircToLower(target) != Client::ircToLower(client.getName(TYPE_NICK)))
    {
        client.sendMessage(ERR_USERSDONTMATCH(client.getName(TYPE_NICK)));
        return;
    }
    client.sendMessage(RPL_UMODEIS(client.getName(TYPE_NICK), "+"));
}

bool Command::prepareChannelMode(Server& server, Client& client, const IRCMessage& msg, Channel*& chan)
{
    if (msg.params.empty() || msg.params[0].empty())
    {
        client.sendMessage(ERR_NEEDMOREPARAMS(
            client.getName(TYPE_NICK), msg.command));
        return false;
    }

    if (!isChannelTarget(msg.params[0]))
    {
        handleUserModeQuery(client, msg.params[0]);
        return false;
    }

    chan = server.getChannel(msg.params[0]);
    if (chan == NULL)
    {
        client.sendMessage(ERR_NOSUCHCHANNEL(
            client.getName(TYPE_NICK), msg.params[0]));
        return false;
    }

    if (msg.params.size() < 2 || msg.params[1].empty())
    {
        client.sendMessage(RPL_CHANNELMODEIS(
            client.getName(TYPE_NICK),
            chan->getName(),
            chan->getModeString()));
        return false;
    }

    if (!chan->isOperator(&client))
    {
        client.sendMessage(ERR_CHANOPRIVSNEEDED(
            client.getName(TYPE_NICK),
            chan->getName()));
        return false;
    }

    return true;
}

static void applyBooleanMode(Channel& chan, bool adding, char mode,
    string& applied, char& lastSign, void (Channel::*setter)(bool))
{
    (chan.*setter)(adding);
    appendApplied(applied, lastSign, adding, mode);
}

static void applyKeyMode(Channel& chan, const IRCMessage& msg, size_t& paramIdx,
    bool adding, string& applied, char& lastSign, string& appliedParams)
{
    if (adding)
    {
        if (paramIdx >= msg.params.size())
            return;
        const string& key = msg.params[paramIdx];
        ++paramIdx;
        if (key.empty())
            return;
        chan.setKey(key);
        appliedParams += " " + key;
    }
    else
        chan.clearKey();
    appendApplied(applied, lastSign, adding, 'k');
}

static bool parsePositiveLong(const string& text, long& out)
{
    char* end = NULL;
    long value = std::strtol(text.c_str(), &end, 10);

    if (end == NULL || *end != '\0' || text.empty() || value <= 0)
        return false;
    out = value;
    return true;
}

static void applyLimitMode(Channel& chan, const IRCMessage& msg, size_t& paramIdx,
    bool adding, string& applied, char& lastSign, string& appliedParams)
{
    if (adding)
    {
        if (paramIdx >= msg.params.size())
            return;
        const string& param = msg.params[paramIdx];
        ++paramIdx;

        long limit = 0;
        if (parsePositiveLong(param, limit))
        {
            chan.setUserLimit(static_cast<size_t>(limit));
            appliedParams += " " + param;
            appendApplied(applied, lastSign, adding, 'l');
        }
    }
    else
    {
        chan.clearUserLimit();
        appendApplied(applied, lastSign, adding, 'l');
    }
}

static void applyOperatorMode(Server& server, Client& client, Channel& chan,
    const IRCMessage& msg, size_t& paramIdx, bool adding,
    string& applied, char& lastSign, string& appliedParams)
{
    if (paramIdx >= msg.params.size())
        return;
    const string& nickParam = msg.params[paramIdx];
    ++paramIdx;

    Client* target = server.getClientByNick(nickParam);
    if (target == NULL)
    {
        client.sendMessage(ERR_NOSUCHNICK(client.getName(TYPE_NICK), nickParam));
        return;
    }
    if (!chan.isMember(target))
    {
        client.sendMessage(ERR_USERNOTINCHANNEL(client.getName(TYPE_NICK),
            target->getName(TYPE_NICK), chan.getName()));
        return;
    }

    if (adding)
        chan.addOperator(target);
    else
        chan.removeOperator(target);
    appliedParams += " " + target->getName(TYPE_NICK);
    appendApplied(applied, lastSign, adding, 'o');
}

void Command::handleMode(Server& server, Client& client, const IRCMessage& msg)
{
    Channel* chan = NULL;

    if (!prepareChannelMode(server, client, msg, chan))
        return;

    const string& modes = msg.params[1];
    size_t paramIdx = 2;
    bool adding = true;
    char lastSign = 0;
    string applied;
    string appliedParams;

    for (size_t i = 0; i < modes.size(); ++i)
    {
        char c = modes[i];

        if (c == '+' || c == '-')
        {
            adding = (c == '+');
            continue;
        }
        switch (c)
        {
        case 'i':
            applyBooleanMode(*chan, adding, 'i', applied, lastSign, &Channel::setInviteOnly);
            break;
        case 't':
            applyBooleanMode(*chan, adding, 't', applied, lastSign, &Channel::setTopicRestricted);
            break;
        case 'k':
            applyKeyMode(*chan, msg, paramIdx, adding, applied, lastSign, appliedParams);
            break;
        case 'l':
            applyLimitMode(*chan, msg, paramIdx, adding, applied, lastSign, appliedParams);
            break;
        case 'o':
            applyOperatorMode(server, client, *chan, msg, paramIdx, adding, applied, lastSign, appliedParams);
            break;
        default:
            client.sendMessage(ERR_UNKNOWNMODE(client.getName(TYPE_NICK), c));
            break;
        }
    }

    if (applied.empty())
        return;
    server.broadcastToChannel(*chan, client.getFullPrefix()
        + " MODE " + chan->getName() + " " + applied + appliedParams + "\r\n");
}
