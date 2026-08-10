#include "Command.hpp"
#include "Server.hpp"

#include <cctype>
#include <sstream>

using std::string;

static bool checkNickname(const string& nick)
{
    string acceptableSymbols = "[]\\`_^{|}";

    if (std::isdigit(static_cast<unsigned char>(nick[0])))
        return false;
    if (nick[0] == ':' || nick[0] == '#' || nick[0] == '&')
        return false;
    for (size_t i = 0; i < nick.size(); ++i)
    {
        if (std::isspace(static_cast<unsigned char>(nick[i])))
            return false;
        if (!std::isalpha(static_cast<unsigned char>(nick[i]))
            && !std::isdigit(static_cast<unsigned char>(nick[i]))
            && acceptableSymbols.find(nick[i]) == string::npos)
            return false;
    }
    return true;
}

static bool nicknameInUse(const Server& server, const string& nick)
{
    const std::map<int, Client>& clients = server.getClients();
    std::map<int, Client>::const_iterator it;

    for (it = clients.begin(); it != clients.end(); ++it)
    {
        if (Client::ircToLower(it->second.getName(TYPE_NICK)) == Client::ircToLower(nick))
            return true;
    }
    return false;
}

void Command::handlePass(Server& server, Client& client, const IRCMessage& msg)
{
    if (msg.params.empty() || msg.params[0].empty())
    {
        client.sendMessage(ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
        return;
    }
    if (client.isFullyRegistered())
    {
        client.sendMessage(ERR_ALREADYREGISTERED(client.getName(TYPE_NICK)));
        return;
    }
    if (msg.params[0] != server.getPassword())
    {
        client.sendMessage(ERR_PASSWDMISMATCH(client.getName(TYPE_NICK)));
        return;
    }
    client.setRegFlag(FLAG_PASS, true);
}

void Command::handleNick(Server& server, Client& client, const IRCMessage& msg)
{
    if (!client.getRegFlag(FLAG_PASS))
    {
        client.sendMessage(ERR_PASSWDMISMATCH(client.getName(TYPE_NICK)));
        return;
    }
    if (msg.params.empty() || msg.params[0].empty())
    {
        client.sendMessage(ERR_NONICKNAMEGIVEN(client.getName(TYPE_NICK)));
        return;
    }
    if (!checkNickname(msg.params[0]))
    {
        client.sendMessage(ERR_ERRONEUSNICKNAME(client.getName(TYPE_NICK), msg.params[0]));
        return;
    }
    if (nicknameInUse(server, msg.params[0]))
    {
        client.sendMessage(ERR_NICKNAMEINUSE(client.getName(TYPE_NICK), msg.params[0]));
        return;
    }
    if (!client.isFullyRegistered())
    {
        client.setName(TYPE_NICK, msg.params[0]);
        client.setRegFlag(FLAG_NICK, true);
        server.checkReg(client);
    }
    else
    {

        string oldPrefix = client.getFullPrefix();
        client.setName(TYPE_NICK, msg.params[0]);
        server.broadcastNickChange(client, oldPrefix);
    }
}

void Command::handleUser(Server& server, Client& client, const IRCMessage& msg)
{
    if (!client.getRegFlag(FLAG_PASS))
    {
        client.sendMessage(ERR_PASSWDMISMATCH(client.getName(TYPE_NICK)));
        return;
    }
    if (client.isFullyRegistered())
    {
        client.sendMessage(ERR_ALREADYREGISTERED(client.getName(TYPE_NICK)));
        return;
    }
    if (msg.params.empty() || msg.params[0].empty() || msg.params.size() < 4)
    {
        client.sendMessage(ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
        return;
    }
    string userlen_str = server.getIsupport("USERLEN");
    std::stringstream ss(userlen_str);
    size_t max_userlen = 0;
    ss >> max_userlen;
    string username = msg.params[0];
    if (max_userlen > 0 && username.length() > max_userlen)
        username.erase(max_userlen);
    client.setName(TYPE_USER, "~" + username);
    client.setName(TYPE_REAL, msg.params.back());
    client.setRegFlag(FLAG_USER, true);
    server.checkReg(client);
}
