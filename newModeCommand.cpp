#include "command.hpp"
#include "Server.hpp"

#include <cstdlib>

using std::string;

// Uygulanan mod karakterini yayin dizgesine ekler; isaret ('+'/'-') yalnizca
// bir onceki eklemeden farkliysa yazilir: "+ik-l" gibi.
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

// Hedef '#' veya '&' ile baslamiyorsa kanal degil kullanicidir.
static bool isChannelTarget(const std::string& target)
{
    return (!target.empty() && (target[0] == '#' || target[0] == '&'));
}

// MODE <nick> icin minimal kullanici-mod sorgusu. irssi kayit sirasinda kendi
// nickini bu sekilde sorgular; tam bir user-mode mutasyon sistemi bu planin
// kapsami disidir. Kendi nicki disinda bir hedef icin kontrollu 502 doner
// (yanlislikla 403 "No such channel" uretmek yerine).
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



// MODE <#kanal>                      -> 324 (mod sorgusu)
// MODE <#kanal> <dizge> [param...]   -> mod degisikligi (yalnizca operatorler)
// MODE <nick>                        -> 221 (minimal kullanici-mod sorgusu)
//
// Dizge +/- isaret durumuyla soldan saga okunur:
//   i, t : parametresiz bayraklar
//   k    : +k anahtar parametresi tuketir; -k tuketmez
//   l    : +l pozitif sayi tuketir (strtol; gecersiz/<=0 yoksayilir); -l tuketmez
//   o    : her iki yonde de nick tuketir (401 / 441 denetimli)
// Parametresi eksik kalan mod sessizce atlanir (461 yerine atlama tercihi).
// Bilinmeyen karakter 472 uretir. Yalnizca gercekten uygulanan degisiklikler
// tek satirda kanala yayinlanir: ":nick!user@host MODE #ch +ik-l key" gibi.
void Command::handleMode(Server& server, Client& client, const IRCMessage& msg)
{
    Channel* chan = NULL;

    if (!prepareChannelMode(server, client, msg, chan))
        return;

    const string& modes = msg.params[1];
    size_t paramIdx = 2;        // parametreler sirayla buradan tuketilir
    bool adding = true;
    char lastSign = 0;
    string applied;             // "+ik-l" — yalniz uygulananlar
    string appliedParams;       // yayina eklenecek parametreler (bosluk onekli)

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
            chan->setInviteOnly(adding);
            appendApplied(applied, lastSign, adding, 'i');
            break;
        case 't':
            chan->setTopicRestricted(adding);
            appendApplied(applied, lastSign, adding, 't');
            break;
        case 'k':
            if (adding)
            {
                if (paramIdx >= msg.params.size())
                    break;                          // anahtarsiz +k atlanir
                if (msg.params[paramIdx].empty())
                {
                    ++paramIdx;                     // bos anahtar: tuket, uygulama
                    break;
                }
                chan->setKey(msg.params[paramIdx]);
                appliedParams += " " + msg.params[paramIdx];
                ++paramIdx;
            }
            else
                chan->clearKey();
            appendApplied(applied, lastSign, adding, 'k');
            break;
        case 'l':
            if (adding)
            {
                if (paramIdx >= msg.params.size())
                    break;                          // sayisiz +l atlanir
                char* end = NULL;
                long limit = std::strtol(msg.params[paramIdx].c_str(), &end, 10);
                bool valid = (end != NULL && *end == '\0'
                    && !msg.params[paramIdx].empty() && limit > 0);
                if (valid)
                {
                    chan->setUserLimit(static_cast<size_t>(limit));
                    appliedParams += " " + msg.params[paramIdx];
                    appendApplied(applied, lastSign, adding, 'l');
                }
                ++paramIdx;                         // gecersiz de olsa tuketildi
            }
            else
            {
                chan->clearUserLimit();
                appendApplied(applied, lastSign, adding, 'l');
            }
            break;
        case 'o':
        {
            if (paramIdx >= msg.params.size())
                break;                              // nick'siz o atlanir
            const string& nickParam = msg.params[paramIdx];
            ++paramIdx;                             // iki yonde de tuketilir
            Client* target = server.getClientByNick(nickParam);
            if (target == NULL)
            {
                client.sendMessage(ERR_NOSUCHNICK(client.getName(TYPE_NICK), nickParam));
                break;
            }
            if (!chan->isMember(target))
            {
                client.sendMessage(ERR_USERNOTINCHANNEL(client.getName(TYPE_NICK),
                    target->getName(TYPE_NICK), chan->getName()));
                break;
            }
            if (adding)
                chan->addOperator(target);
            else
                chan->removeOperator(target);
            appliedParams += " " + target->getName(TYPE_NICK);
            appendApplied(applied, lastSign, adding, 'o');
            break;
        }
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
