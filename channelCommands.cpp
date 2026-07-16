#include "command.hpp"
#include "Server.hpp"

using std::string;

// Kanal adi kurallari (RFC 2812): '#' veya '&' ile baslar, prefix disinda en
// az 1, toplam en fazla 50 karakter; bosluk, virgul ve BEL (^G) iceremez.
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

void Command::handleJoin(Server&, Client& client, const IRCMessage& msg)
{
    if (msg.params.empty() || msg.params[0].empty())
    {
        client.sendMessage(ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
        return;
    }
    const string& name = msg.params[0];
    if (!isValidChannelName(name))
    {
        client.sendMessage(ERR_BADCHANMASK(client.getName(TYPE_NICK), name));
        return;
    }
    // TODO (sonraki adimlar):
    //  - mod denetimleri: +k (475), +l (471), +i (473)
    //  - getOrCreateChannel + addMember + ilk uyeye op + davet tuketimi
    //  - JOIN broadcast + RPL_TOPIC / RPL_TOPICWHOTIME + NAMES (353/366)
    //  - virgulle ayrilmis coklu kanal ve anahtar listesi
}
