#include "Server.hpp"
#include "command.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <csignal>
#include <cctype>
#include <ctime>
#include <set>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::runtime_error;
using std::map;
using std::set;

static volatile sig_atomic_t g_running = 1;

static void signalHandler(int)
{
    g_running = 0;
}

Server::Server(const string& port, const string& password)
    : _serverFd(-1), _port(0), _password(password)
{
    char* end = NULL;
    long value = std::strtol(port.c_str(), &end, 10);
    if (*end != '\0' || value < 1 || value > 65535)
        throw runtime_error("invalid port");
    if (_password.empty())
        throw runtime_error("password cannot be empty");
    _port = static_cast<int>(value);
    initCommands();
    initIsupport();

    std::time_t now = std::time(NULL);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    _creationDate = buf;
}

Server::~Server()
{
    for (size_t i = 1; i < _pollFds.size(); ++i)
    {
        if (_pollFds[i].fd != -1)
            close(_pollFds[i].fd);
    }
    if (_serverFd != -1)
        close(_serverFd);
}

void Server::start()
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    // send() bir kapanmis peer'a yazarsa SIGPIPE process'i sonlandirabilir;
    // hata zaten EPIPE olarak send() donusunden okunuyor, bu yuzden sinyali yoksay.
    signal(SIGPIPE, SIG_IGN);
    setupSocket();
    cout << "IRC server listening on port " << _port << endl;
    while (g_running)
    {
        int ready = poll(&_pollFds[0], _pollFds.size(), -1);
        if (ready < 0)
        {
            if (errno == EINTR) continue;
            throw runtime_error(string("poll failed: ") + strerror(errno));
        }
        for (size_t i = 0; i < _pollFds.size() && ready > 0; ++i)
            handlePollEvents(i, ready);
        // Bu turun tum revents'leri islendi: ne _pollFds/_clients uzerinde
        // aktif bir iterasyon ne de bir handler call-stack'i var. Deferred
        // removal'lari flush etmek icin tek guvenli nokta burasidir.
        processPendingRemovals();
    }
    cout << "Server stopped" << endl;
}

// POLLIN ve POLLHUP/POLLERR ayni turda birlikte gelebilir: peer son verisini
// yazip hemen kapatirsa kernel her ikisini de tek poll() donusunde bildirir.
// Bu yuzden HUP/ERR/NVAL asla POLLIN'den ONCE islenmez -- once elde ne varsa
// okunur (readFromClient kendi ic dongusunde veri bitene/EAGAIN'e kadar
// recv eder), ardindan gerekiyorsa POLLOUT, en sonda hala hayattaysa
// HUP/ERR/NVAL uzerinden temizlik yapilir. readFromClient zaten recv()==0
// gordugunde removeClient() cagirir; asagidaki `_pollFds[i].fd == fd`
// kontrolleri bu durumda ayni fd uzerinde tekrar islem yapilmasini engeller.
void Server::handlePollEvents(size_t& i, int& ready)
{
    int fd = _pollFds[i].fd;
    short revents = _pollFds[i].revents;
    if (revents == 0)
		return;
    --ready;
    if (fd == _serverFd)
	{
        if (revents & POLLIN)
			acceptClients();
    }
	else
	{
        if (revents & POLLIN)
			readFromClient(fd);
        if (i < _pollFds.size() && _pollFds[i].fd == fd && (revents & POLLOUT))
            writeToClient(fd);
        if (i < _pollFds.size() && _pollFds[i].fd == fd && (revents & (POLLERR | POLLHUP | POLLNVAL)))
            removeClient(fd);
    }
    if (i >= _pollFds.size() || _pollFds[i].fd != fd)
	{
        if (i > 0)
			--i;
    }
}

void Server::setupSocket()
{
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0)
        throw runtime_error(string("socket failed: ") + strerror(errno));
    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw runtime_error(string("setsockopt failed: ") + strerror(errno));
    setNonBlocking(_serverFd);
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(_port);
    if (bind(_serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw runtime_error(string("bind failed: ") + strerror(errno));
    if (listen(_serverFd, SOMAXCONN) < 0)
        throw runtime_error(string("listen failed: ") + strerror(errno));
    addPollFd(_serverFd, POLLIN);
}

void Server::setNonBlocking(int fd)
{
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
        throw runtime_error(string("fcntl failed: ") + strerror(errno));
}

void Server::addPollFd(int fd, short events)
{
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = events;
    pfd.revents = 0;
    _pollFds.push_back(pfd);
}

// Baglanti kapanirken istemci tum kanallardan cikarilir; kalan uyeler QUIT
// bildirimi alir, bosalan kanal silinir. Bu temizlik Client nesnesi hala
// hayattayken (erase'ten ONCE) yapilmali, yoksa kanallardaki Client*
// gozlemcileri serbest birakilmis bellege isaret eder (dangling pointer).
//
// Davet temizligi uyelikten bagimsiz olarak HER kanal icin yapilir: bir
// davetli hicbir zaman uye olmamis olabilir, bu yuzden yalniz uyelik uzerinden
// (chan.isMember kontrolu gecen kanallar) gidilemez. Bu adim atlanirsa, fd
// yeniden kullanildiginda eski davet yeni istemciye devreder (+i bypass).
void Server::removeClientFromChannels(Client& client)
{
    string quitMsg = client.getFullPrefix()
        + " QUIT :" + client.getQuitReason() + "\r\n";
    set<int> recipients;
    map<string, Channel>::iterator it = _channels.begin();
    while (it != _channels.end())
    {
        Channel& chan = it->second;
        chan.removeInvite(&client);
        if (chan.isMember(&client))
        {
            // Yayin alacaklari uyelik silinmeden ONCE topla (kendisi haric);
            // ayni alici birden fazla ortak kanal paylasabileceginden fd'ler
            // set ile tekillestirilir, QUIT mesaji her aliciya tek sefer gider.
            const map<int, Client*>& members = chan.getMembers();
            for (map<int, Client*>::const_iterator mit = members.begin();
                mit != members.end(); ++mit)
            {
                if (mit->first != client.getFd())
                    recipients.insert(mit->first);
            }

            chan.removeMember(&client);
            if (chan.isEmpty())
            {
                _channels.erase(it++);
                continue;
            }
        }
        ++it;
    }

    for (set<int>::const_iterator rit = recipients.begin();
        rit != recipients.end(); ++rit)
    {
        map<int, Client>::iterator cit = _clients.find(*rit);
        if (cit != _clients.end())
            sendToClient(cit->second, quitMsg);
    }
}

void Server::removeClient(int fd)
{
    map<int, Client>::iterator cit = _clients.find(fd);
    if (cit == _clients.end())
        return ;    // zaten kaldirilmis (ör. ayni turda baska bir yoldan); no-op
    removeClientFromChannels(cit->second);
    cout << "Client disconnected fd=" << fd << endl;
    close(fd);
    for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
    {
        if (it->fd == fd)
        {
            _pollFds.erase(it);
            break;
        }
    }
    _clients.erase(fd);
}

// Bir istemciyi HEMEN silmek yerine yalniz "silinmeli" olarak isaretler.
// Cagiran taraf (sendToClient/broadcastToChannel) bir Channel/Client
// container iterasyonunun veya derin bir handler call-stack'inin ortasinda
// olabilir; gercek removeClient() orada guvenli degildir. set fd'yi
// dogal olarak tekillestirir, ayni istemci birden fazla kez isaretlense de
// tek silme islemi yapilir.
void Server::requestRemoval(int fd)
{
    if (_clients.find(fd) != _clients.end())
        _pendingRemovals.insert(fd);
}

// Her poll turunun sonunda cagrilir. Once _clients'daki TUM istemciler
// output-overflow acisindan taranir: bu, cagri yoluna bakmaksizin (ister
// sendToClient/broadcastToChannel uzerinden, ister bir handler'in dogrudan
// client.sendMessage() cagrisindan) olusan her overflow'u yakalar. Ardindan
// isaretli fd'ler icin gercek removeClient() bu guvenli noktada calisir.
void Server::processPendingRemovals()
{
    for (map<int, Client>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second.outputOverflow())
            _pendingRemovals.insert(it->first);
    }
    // removeClient() -> removeClientFromChannels() -> broadcastToChannel()
    // zincirinde kalan uyelere QUIT yayinlanirken yeni requestRemoval()
    // cagrilari olusabilir (ör. o yayinla dolan baska bir slow-reader).
    // Ayni set uzerinde iterasyon yaparken bunu eklemek UB degildir, ama
    // hangi elemanin bu turda islenip hangisinin ertelendigi konum bagimli
    // hale gelir. Bunun yerine her batch ayri bir kopyaya swap edilip
    // tuketilir; bu sirada olusan yeni istekler _pendingRemovals'ta acikca
    // bir SONRAKI batch'e kalir.
    while (!_pendingRemovals.empty())
    {
        set<int> pending;
        pending.swap(_pendingRemovals);
        for (set<int>::const_iterator it = pending.begin(); it != pending.end(); ++it)
            removeClient(*it);
    }
}

void Server::acceptClients()
{
    while (true)
    {
        sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int fd = accept(_serverFd, reinterpret_cast<sockaddr*>(&addr), &len);
        if (fd < 0)
		{
            if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
            cerr << "accept failed: " << strerror(errno) << endl;
            break;
        }
        try
		{
            setNonBlocking(fd);
        } 
		catch (const std::exception& e)
		{
            // Tek bir accepted fd icin setup hatasi fatal degildir; yalniz bu
            // baglanti reddedilir, event loop calismaya devam eder. Listener
            // setup hatasi (setupSocket) ayri bir yoldur ve fatal kalir.
            cerr << "client fd=" << fd << " setup failed: " << e.what() << endl;
            close(fd);
            continue;
        }
        addPollFd(fd, POLLIN);
        map<int, Client>::iterator res = _clients.insert(std::make_pair(fd, Client(fd))).first;
        res->second.setHost(inet_ntoa(addr.sin_addr));
        cout << "New client fd=" << fd << " ip=" << inet_ntoa(addr.sin_addr) << endl;
    }
}

void Server::readFromClient(int fd)
{
    char buffer[512];
    while (true)
	{
        std::memset(buffer, 0, sizeof(buffer));
        ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
        if (bytes > 0) 
		{
            map<int, Client>::iterator it = _clients.find(fd);
            if (it == _clients.end())
                return;
            it->second.appendToBuffer(string(buffer, bytes));
            while (true)
			{
                map<int, Client>::iterator cit = _clients.find(fd);
                if (cit == _clients.end() || !cit->second.hasCommands())
                    break;
                IRCMessage msg = cit->second.getNextCommand();
                handleCommand(cit->second, msg);
            }
            if (_clients.find(fd) == _clients.end())
                return;
        }
		else if (bytes == 0)
		{
            removeClient(fd);
            return;
        }
		else
		{
            if (errno == EINTR)
				continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
            cerr << "recv failed fd=" << fd << ": " << strerror(errno) << endl;
            removeClient(fd);
            return;
        }
    }
    if (_clients.find(fd) != _clients.end())
        updatePollEvents(fd);
}

void Server::writeToClient(int fd)
{
    map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;
    Client& client = it->second;
    if (!client.hasOutput())
	{
        updatePollEvents(fd);
        if (client.closeAfterWrite())
            removeClient(fd);
        return;
    }
    const string& out = client.outBuffer();
    ssize_t sent;
    do
	{
        sent = send(fd, out.c_str(), out.size(), 0);
    }
	while (sent < 0 && errno == EINTR);
    if (sent > 0)
	{
        client.consumeOutput(static_cast<std::size_t>(sent));
        if (!client.hasOutput() && client.closeAfterWrite())
		{
            removeClient(fd);
            return;
        }
        updatePollEvents(fd);
    }
	else if (sent < 0)
	{
        if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
        cerr << "send failed fd=" << fd << ": " << strerror(errno) << endl;
        removeClient(fd);
    }
	else
	{
        // sent == 0 iken pending output var: ilerleme yok, baglanti
        // kullanilamaz durumda. Sonsuz POLLOUT dongusune girmemek icin
        // istemciyi kaldir.
        removeClient(fd);
    }
}

void Server::updatePollEvents(int fd)
{
    map<int, Client>::iterator cit = _clients.find(fd);
    for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
    {
        if (it->fd == fd)
        {
            it->events = POLLIN;
            if (cit != _clients.end() && cit->second.hasOutput())
                it->events |= POLLOUT;
            break;
        }
    }
}

void Server::initCommands()
{
    _commands["CAP"] = &Command::handleCap;
    _commands["PING"] = &Command::handlePing;
    _commands["QUIT"] = &Command::handleQuit;
    _commands["PASS"] = &Command::handlePass;
    _commands["NICK"] = &Command::handleNick;
    _commands["USER"] = &Command::handleUser;
    _commands["MOTD"] = &Command::handleMotd;
    _commands["WHO"] = &Command::handleWho;
    _commands["PRIVMSG"] = &Command::handlePrivmsg;
    _commands["NOTICE"] = &Command::handleNotice;
    _commands["JOIN"] = &Command::handleJoin;
    _commands["PART"] = &Command::handlePart;
    _commands["TOPIC"] = &Command::handleTopic;
    _commands["MODE"] = &Command::handleMode;
    _commands["KICK"] = &Command::handleKick;
    _commands["INVITE"] = &Command::handleInvite;
}

/* Komut dagitimi: komut adi (case-insensitive) tabloda aranir; bulunursa
   ilgili Command:: statik handler'i cagrilir. */
void Server::handleCommand(Client& client, const IRCMessage& msg)
{
    cout << "fd=" << client.getFd() << " > " << msg.command << endl;
    string cmd = Command::upper(msg.command);
    map<string, CmdFunc>::iterator it = _commands.find(cmd);
    if (it == _commands.end())
    {
        client.sendMessage(ERR_UNKNOWNCOMMAND(client.getName(TYPE_NICK), msg.command));
        return;
    }
    // Kayit kapisi: kayitli olmayan istemci yalnizca el sikisma/kayit
    // komutlarini kullanabilir; digerleri icin ERR_NOTREGISTERED (451).
    // CAP/PING/QUIT muaf: istemciler CAP LS'i PASS'tan once gonderir.
    if (!client.isFullyRegistered()
        && cmd != "PASS" && cmd != "NICK" && cmd != "USER"
        && cmd != "CAP" && cmd != "PING" && cmd != "QUIT")
    {
        client.sendMessage(ERR_NOTREGISTERED(client.getName(TYPE_NICK)));
        return;
    }
    it->second(*this, client, msg);
}

void Server::initIsupport()
{
    _isupport["USERLEN"] = "15";
}

const string& Server::getPassword() const
{
    return _password;
}

map<int, Client>& Server::getClients()
{
    return _clients;
}

const map<int, Client>& Server::getClients() const
{
    return _clients;
}

string Server::getIsupport(const string& key) const
{
    map<string, string>::const_iterator it = _isupport.find(key);
    if (it == _isupport.end())
        return "";
    return it->second;
}

const string& Server::getCreationDate() const
{
    return _creationDate;
}

// Bir handler baska bir istemciye mesaj gonderdiginde, hedef fd icin
// POLLOUT'u hemen etkinlestirir. Boylece hedefin yeni bir komut yazmasini
// beklemeden tamponlu cikis poll dongusu tarafindan bosaltilir.
void Server::sendToClient(Client& client, const string& message)
{
    int fd = client.getFd();
    client.sendMessage(message);
    if (client.outputOverflow())
	{
        requestRemoval(fd);
        return;
    }
    updatePollEvents(fd);
}

// Kayit tamamlandiginda (PASS+NICK+USER) hos geldin numeric'lerini gonderir.
// Yalnizca client'i degistirir; Server durumunu okur (creationDate), o yuzden const.
void Server::checkReg(Client& client) const
{
    if (client.getRegFlag(FLAG_REGISTERED))
        return;
    if (!client.isFullyRegistered())
        return;
    client.setRegFlag(FLAG_REGISTERED, true);
    client.sendMessage(RPL_WELCOME(client.getName(TYPE_NICK), client.getName(TYPE_USER), client.getHost()));
    client.sendMessage(RPL_YOURHOST(client.getName(TYPE_NICK), "ircserv 1.0"));
    client.sendMessage(RPL_CREATED(client.getName(TYPE_NICK), getCreationDate()));
    // RPL_MYINFO servername'i kendi icinde ekliyor; version alanina ayrica
    // "ircserv" gecirmek servername'i tekrarlar ve bosluk yuzunden fazladan
    // bir token gibi gorunur (ör. "ircserv ircserv 1.0 ..."). Version tek
    // basina, bosluksuz tek token olmali.
    client.sendMessage(RPL_MYINFO(client.getName(TYPE_NICK), "1.0", "i", "t,k,l"));
}

// Nick'ler case-insensitive eslestirilir. PRIVMSG/NOTICE ve MODE +o/-o,
// KICK, INVITE gibi nick hedefli komutlarin ortak aramasi.
Client* Server::getClientByNick(const string& nick)
{
    string lowered = Client::ircToLower(nick);
    map<int, Client>::iterator it;

    for (it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (Client::ircToLower(it->second.getName(TYPE_NICK)) == lowered)
            return &it->second;
    }
    return NULL;
}

// Kanal adlari case-insensitive'dir: harita anahtari ircToLower ile normalize
// edilir; kanalin gorunen adi ise ilk olusturanin yazdigi haliyle Channel
// icinde saklanir.
Channel* Server::getChannel(const string& name)
{
    map<string, Channel>::iterator it = _channels.find(Client::ircToLower(name));
    if (it == _channels.end())
        return NULL;
    return &it->second;
}

// Channel'in default constructor'u yok; bu yuzden operator[] derlenmez,
// find + insert kullanilir.
Channel& Server::getOrCreateChannel(const string& name)
{
    string key = Client::ircToLower(name);
    map<string, Channel>::iterator it = _channels.find(key);
    if (it == _channels.end())
        it = _channels.insert(std::make_pair(key, Channel(name))).first;
    return it->second;
}

// Son uye ayrildiginda kanali kaldirir (PART/QUIT/KICK sonrasinda cagrilir).
void Server::removeEmptyChannel(const string& name)
{
    map<string, Channel>::iterator it = _channels.find(Client::ircToLower(name));
    if (it != _channels.end() && it->second.isEmpty())
        _channels.erase(it);
}

// Kanala yayin: Channel::broadcast mesaji uyelerin tamponina yazar; ardindan
// her uyenin fd'si icin POLLOUT etkinlestirilir ki mesajlar hedefler kendi
// olayini beklemeden aksin (sendToClient'in kanal karsiligi).
void Server::broadcastToChannel(const Channel& channel, const string& message, Client* except)
{
    channel.broadcast(message, except);
    const map<int, Client*>& members = channel.getMembers();
    // Asiri dolan (slow-reader) uyeler burada yalniz ISARETLENIR
    // (requestRemoval), silinmez: Channel::_members uzerinde hala iterasyon
    // halindeyiz ve gercek removeClient() bu haritayi (removeMember uzerinden)
    // degistirirdi. requestRemoval yalniz _pendingRemovals kumesine ekler,
    // hicbir Channel/Client container'ini mutasyona ugratmadigi icin bu
    // dongu icinde cagrilmasi guvenlidir.
    for (map<int, Client*>::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        updatePollEvents(it->first);
        if (it->second != NULL && it->second->outputOverflow())
            requestRemoval(it->first);
    }
}

// Kayitli bir kullanici nick degistirdiginde ortak kanal uyelerine NICK
// event'ini yayinlar. Ayni alici birden fazla ortak kanal paylasabileceginden
// fd'ler set ile tekillestirilir; degisen kullanici event'i en sonda ve
// tek sefer alir. oldPrefix cagiran tarafindan (mutasyondan ONCE) yakalanir,
// cunku bu fonksiyon yeni nick'i client.getName() uzerinden okur.
void Server::broadcastNickChange(Client& client, const string& oldPrefix)
{
    string nickMsg = oldPrefix + " NICK " + client.getName(TYPE_NICK) + "\r\n";
    set<int> recipients;

    for (map<string, Channel>::const_iterator cit = _channels.begin();
        cit != _channels.end(); ++cit)
    {
        if (!cit->second.isMember(&client))
            continue;
        const map<int, Client*>& members = cit->second.getMembers();
        for (map<int, Client*>::const_iterator mit = members.begin();
            mit != members.end(); ++mit)
        {
            if (mit->first != client.getFd())
                recipients.insert(mit->first);
        }
    }

    for (set<int>::const_iterator it = recipients.begin(); it != recipients.end(); ++it)
    {
        map<int, Client>::iterator rit = _clients.find(*it);
        if (rit != _clients.end())
            sendToClient(rit->second, nickMsg);
    }
    sendToClient(client, nickMsg);
}
