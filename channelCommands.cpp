#include "command.hpp"
#include "Server.hpp"

#include <sstream>
#include <vector>

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

// Virgulle ayrilmis listeyi parcalarina ayirir. Bos parcalar KORUNUR: anahtar
// listesinde ",k2" gibi bos yuvalar konumsal olarak anlamlidir.
static std::vector<string> splitList(const string& list)
{
    std::vector<string> out;
    std::stringstream ss(list);
    string item;

    while (std::getline(ss, item, ','))
        out.push_back(item);
    return out;
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

// Tek bir kanala katilma akisi: dogrulama, giris denetimleri (+i/+k/+l),
// uyelik + ilk uyeye operatorluk, JOIN yayini ve topic/NAMES cevaplari.
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
}

// JOIN <#kanal>{,<#kanal>} [<anahtar>{,<anahtar>}]
// Anahtarlar kanallarla konumsal eslesir; eksik kalanlar bos sayilir.
void Command::handleJoin(Server& server, Client& client, const IRCMessage& msg)
{
    if (msg.params.empty() || msg.params[0].empty())
    {
        client.sendMessage(ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
        return;
    }

    std::vector<string> names = splitList(msg.params[0]);
    std::vector<string> keys;
    if (msg.params.size() >= 2)
        keys = splitList(msg.params[1]);

    for (size_t i = 0; i < names.size(); ++i)
    {
        if (names[i].empty())
            continue;
        joinOne(server, client, names[i], i < keys.size() ? keys[i] : string());
    }
}
