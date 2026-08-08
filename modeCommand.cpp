#include "command.hpp"
#include "Server.hpp"

#include <cstdlib>

using std::string;
using std::strtol;
using std::size_t;

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

static bool isChannelTarget(const string& target)
{
    return (!target.empty() && (target[0] == '#' || target[0] == '&'));
}

static void handleUserModeQuery(Client& client, const string& target)
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

// --- Mod Yardımcı Fonksiyonları ---

static void processModeI(Channel* chan, bool adding, char& lastSign, string& applied)
{
    chan->setInviteOnly(adding);
    appendApplied(applied, lastSign, adding, 'i');
}

static void processModeT(Channel* chan, bool adding, char& lastSign, string& applied)
{
    chan->setTopicRestricted(adding);
    appendApplied(applied, lastSign, adding, 't');
}

static void processModeK(Channel* chan, const IRCMessage& msg, size_t& paramIdx, bool adding, char& lastSign, string& applied, string& appliedParams)
{
    if (adding)
    {
        if (paramIdx >= msg.params.size())
            return;
        if (msg.params[paramIdx].empty())
        {
            ++paramIdx;
            return;
        }
        chan->setKey(msg.params[paramIdx]);
        appliedParams += " " + msg.params[paramIdx];
        ++paramIdx;
    }
    else
    {
        chan->clearKey();
    }
    appendApplied(applied, lastSign, adding, 'k');
}

static void processModeL(Channel* chan, const IRCMessage& msg, size_t& paramIdx, bool adding, char& lastSign, string& applied, string& appliedParams)
{
    if (adding)
    {
        if (paramIdx >= msg.params.size())
            return;
        char* end = NULL;
        long limit = strtol(msg.params[paramIdx].c_str(), &end, 10);
        bool valid = (end != NULL && *end == '\0' && !msg.params[paramIdx].empty() && limit > 0);
        if (valid)
        {
            chan->setUserLimit(static_cast<size_t>(limit));
            appliedParams += " " + msg.params[paramIdx];
            appendApplied(applied, lastSign, adding, 'l');
        }
        ++paramIdx;
    }
    else
    {
        chan->clearUserLimit();
        appendApplied(applied, lastSign, adding, 'l');
    }
}

static void processModeO(Server& server, Client& client, Channel* chan, const IRCMessage& msg, size_t& paramIdx, bool adding, char& lastSign, string& applied, string& appliedParams)
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
    if (!chan->isMember(target))
    {
        client.sendMessage(ERR_USERNOTINCHANNEL(client.getName(TYPE_NICK),
            target->getName(TYPE_NICK), chan->getName()));
        return;
    }
    if (adding)
        chan->addOperator(target);
    else
        chan->removeOperator(target);
        
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
                processModeI(chan, adding, lastSign, applied);
                break;
            case 't':
                processModeT(chan, adding, lastSign, applied);
                break;
            case 'k':
                processModeK(chan, msg, paramIdx, adding, lastSign, applied, appliedParams);
                break;
            case 'l':
                processModeL(chan, msg, paramIdx, adding, lastSign, applied, appliedParams);
                break;
            case 'o':
                processModeO(server, client, chan, msg, paramIdx, adding, lastSign, applied, appliedParams);
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
