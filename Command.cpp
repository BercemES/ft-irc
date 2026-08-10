#include "Command.hpp"

#include <cctype>

std::string Command::upper(const std::string& str)
{
    std::string out = str;
    for (std::size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[i])));
    return out;
}

void Command::handleCap(Server&, Client& client, const IRCMessage& msg)
{
    std::string sub;
    if (!msg.params.empty())
        sub = upper(msg.params[0]);
    if (sub == "END")
        return;
    if (sub == "REQ")
        client.sendMessage(":ircserv CAP * NAK :\r\n");
    else
        client.sendMessage(":ircserv CAP * LS :\r\n");
}

void Command::handlePing(Server&, Client& client, const IRCMessage& msg)
{
    std::string payload;
    if (!msg.params.empty())
        payload = msg.params[0];
    if (payload.empty())
        payload = "ircserv";
    client.sendMessage("PONG :" + payload + "\r\n");
}

void Command::handleQuit(Server&, Client& client, const IRCMessage& msg)
{
    std::string reason;
    if (!msg.params.empty())
        reason = msg.params[0];

    std::string errorMsg = "ERROR :Closing Link. Bye for now!";
    if (!reason.empty())
    {
        client.setQuitReason("Quit: " + reason);
        errorMsg += " (Quit: " + reason + ")";
    }
    client.sendMessage(errorMsg + "\r\n");
    client.setCloseAfterWrite(true);
}
