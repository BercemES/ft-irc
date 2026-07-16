#include "command.hpp"
#include "Server.hpp"

#include <sstream>

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

// RPL_NAMREPLY (353) icin uye listesini uretir; operatorler '@' on ekiyle.
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

void Command::handleJoin(Server& server, Client& client, const IRCMessage& msg)
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

    string key;
    if (msg.params.size() >= 2)
        key = msg.params[1];

    // Kanal varsa giris denetimleri (+i/+k/+l) uygulanir; yoksa olusturulur
    // ve kanali acan ilk uye operator olur.
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

    chan->addMember(&client);       // bekleyen davet burada tuketilir
    if (created)
        chan->addOperator(&client);

    // JOIN, kanalin tum uyelerine (girene de onay olarak) yayinlanir.
    string prefix = ":" + client.getName(TYPE_NICK) + "!"
        + client.getName(TYPE_USER) + "@" + client.getHost();
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

    // TODO: virgulle ayrilmis coklu kanal + anahtar listesi (son adim).
}
