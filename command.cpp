#include "command.hpp"

#include <cctype>

// Komut adlarini eslestirmek icin ASCII buyuk-harfe cevirir (komutlar
// case-insensitive). Handler'lar Server'i kullanmadigi icin parametre adsiz.
std::string Command::upper(const std::string& str)
{
    std::string out = str;
    for (std::size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[i])));
    return out;
}

// CAP: minimal muzakere. LS/REQ/END disindakilere de kayit akisini
// bloklamamak icin LS ile cevap veriyoruz.
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

void Command::handleQuit(Server&, Client& client, const IRCMessage&)
{
    client.sendMessage("ERROR :Closing Link\r\n");
    client.setCloseAfterWrite(true);
}
