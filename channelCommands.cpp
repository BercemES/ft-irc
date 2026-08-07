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

// Tek bir kanaldan ayrilma akisi: dogrulama, PART yayini (ayrilan da onay
// olarak alir), uyelik silme ve bosalan kanalin kaldirilmasi.
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

    // Yayin uyelik silinmeden YAPILIR ki ayrilan da onayini alsin.
    string prefix = client.getFullPrefix();
    server.broadcastToChannel(*chan, prefix + " PART " + chan->getName()
        + (reason.empty() ? "" : " :" + reason) + "\r\n");

    chan->removeMember(&client);
    server.removeEmptyChannel(name);
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

// PART <#kanal>{,<#kanal>} [:<sebep>]
void Command::handlePart(Server& server, Client& client, const IRCMessage& msg)
{
    if (msg.params.empty() || msg.params[0].empty())
    {
        client.sendMessage(ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
        return;
    }

    std::vector<string> names = splitList(msg.params[0]);
    string reason = msg.params.size() >= 2 ? msg.params[1] : string();

    for (size_t i = 0; i < names.size(); ++i)
    {
        if (names[i].empty())
            continue;
        partOne(server, client, names[i], reason);
    }
}

// TOPIC <#kanal> [:<konu>]
// Ikinci parametre yoksa sorgu (331 veya 332+333); varsa degistirme. Bos konu
// metni konuyu siler (hasTopic bos metinde false doner).
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

    if (msg.params.size() < 2)  // sorgu
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

    // degistirme: +t iken operator olmayan uye reddedilir
    if (chan->isTopicRestricted() && !chan->isOperator(&client))
    {
        client.sendMessage(ERR_CHANOPRIVSNEEDED(client.getName(TYPE_NICK), chan->getName()));
        return;
    }
    chan->setTopic(msg.params[1], client.getName(TYPE_NICK));
    server.broadcastToChannel(*chan, client.getFullPrefix()
        + " TOPIC " + chan->getName() + " :" + msg.params[1] + "\r\n");
}

// KICK <#kanal> <nick> [:<sebep>]
// Denetim sirasi: 461 -> 403 -> 442 (kickleyen uye degil) -> 482 (op degil)
// -> 441 (hedef kanalda degil; bilinmeyen nick de bu yola duser).
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

    // Yayin uyelik silinmeden YAPILIR ki atilan da KICK'i gorsun.
    server.broadcastToChannel(*chan, client.getFullPrefix()
        + " KICK " + chan->getName() + " " + target->getName(TYPE_NICK)
        + " :" + reason + "\r\n");

    chan->removeMember(target);
    server.removeEmptyChannel(msg.params[0]);
}

// INVITE <nick> <#kanal>
// Denetim sirasi: 461 -> 403 -> 442 (davet eden uye degil) -> 482 (davet eden
// operator degil; +i sadece JOIN'in davet gerektirip gerektirmedigini belirler)
// -> 401 (hedef nick yok) -> 443 (hedef zaten uye). Davetler tek kullanimlik:
// Channel::addMember JOIN sirasinda daveti tuketir.
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
