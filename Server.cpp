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

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static volatile sig_atomic_t g_running = 1;

static void signalHandler(int)
{
    g_running = 0;
}

Server::Server(const std::string& port, const std::string& password)
    : _serverFd(-1), _port(0), _password(password)
{
    char* end = NULL;
    long value = std::strtol(port.c_str(), &end, 10);
    if (*end != '\0' || value < 1 || value > 65535)
        throw std::runtime_error("invalid port");
    if (_password.empty())
        throw std::runtime_error("password cannot be empty");
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
    setupSocket();
    std::cout << "IRC server listening on port " << _port << std::endl;
    while (g_running)
    {
        int ready = poll(&_pollFds[0], _pollFds.size(), -1);
        if (ready < 0)
        {
            if (errno == EINTR) continue;
            throw std::runtime_error(std::string("poll failed: ") + strerror(errno));
        }
        for (size_t i = 0; i < _pollFds.size() && ready > 0; ++i)
            handlePollEvents(i, ready);
    }
    std::cout << "Server stopped" << std::endl;
}

void Server::handlePollEvents(size_t& i, int& ready)
{
    int fd = _pollFds[i].fd;
    short revents = _pollFds[i].revents;
    if (revents == 0) return;
    --ready;
    if (fd == _serverFd) {
        if (revents & POLLIN) acceptClients();
    } else if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
        removeClient(fd);
    } else {
        if (revents & POLLIN) readFromClient(fd);
        if (i < _pollFds.size() && _pollFds[i].fd == fd && (revents & POLLOUT))
            writeToClient(fd);
    }
    if (i >= _pollFds.size() || _pollFds[i].fd != fd) {
        if (i > 0) --i;
    }
}

void Server::setupSocket()
{
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0)
        throw std::runtime_error(std::string("socket failed: ") + strerror(errno));
    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error(std::string("setsockopt failed: ") + strerror(errno));
    setNonBlocking(_serverFd);
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(_port);
    if (bind(_serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error(std::string("bind failed: ") + strerror(errno));
    if (listen(_serverFd, SOMAXCONN) < 0)
        throw std::runtime_error(std::string("listen failed: ") + strerror(errno));
    addPollFd(_serverFd, POLLIN);
}

void Server::setNonBlocking(int fd)
{
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error(std::string("fcntl failed: ") + strerror(errno));
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
void Server::removeClientFromChannels(Client& client)
{
    std::string quitMsg = ":" + client.getName(TYPE_NICK) + "!"
        + client.getName(TYPE_USER) + "@" + client.getHost()
        + " QUIT :Client Quit\r\n";
    std::map<std::string, Channel>::iterator it = _channels.begin();
    while (it != _channels.end())
    {
        Channel& chan = it->second;
        if (chan.isMember(&client))
        {
            chan.removeMember(&client);
            if (chan.isEmpty())
            {
                _channels.erase(it++);
                continue;
            }
            broadcastToChannel(chan, quitMsg);
        }
        ++it;
    }
}

void Server::removeClient(int fd)
{
    std::map<int, Client>::iterator cit = _clients.find(fd);
    if (cit != _clients.end())
        removeClientFromChannels(cit->second);
    std::cout << "Client disconnected fd=" << fd << std::endl;
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

void Server::acceptClients()
{
    while (true)
    {
        sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int fd = accept(_serverFd, reinterpret_cast<sockaddr*>(&addr), &len);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            std::cerr << "accept failed: " << strerror(errno) << std::endl;
            break;
        }
        setNonBlocking(fd);
        addPollFd(fd, POLLIN);
        std::map<int, Client>::iterator res = _clients.insert(std::make_pair(fd, Client(fd))).first;
        res->second.setHost(inet_ntoa(addr.sin_addr));
        std::cout << "New client fd=" << fd << " ip=" << inet_ntoa(addr.sin_addr) << std::endl;
    }
}

void Server::readFromClient(int fd)
{
    char buffer[512];
    while (true) {
        std::memset(buffer, 0, sizeof(buffer));
        ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
        if (bytes > 0) {
            std::map<int, Client>::iterator it = _clients.find(fd);
            if (it == _clients.end())
                return;
            it->second.appendToBuffer(std::string(buffer, bytes));
            while (true) {
                std::map<int, Client>::iterator cit = _clients.find(fd);
                if (cit == _clients.end() || !cit->second.hasCommands())
                    break;
                IRCMessage msg = cit->second.getNextCommand();
                handleCommand(cit->second, msg);
            }
            if (_clients.find(fd) == _clients.end())
                return;
        } else if (bytes == 0) {
            removeClient(fd);
            return;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            std::cerr << "recv failed fd=" << fd << ": " << strerror(errno) << std::endl;
            removeClient(fd);
            return;
        }
    }
    if (_clients.find(fd) != _clients.end())
        updatePollEvents(fd);
}

void Server::writeToClient(int fd)
{
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;
    Client& client = it->second;
    if (!client.hasOutput()) {
        updatePollEvents(fd);
        if (client.closeAfterWrite())
            removeClient(fd);
        return;
    }
    const std::string& out = client.outBuffer();
    ssize_t sent = send(fd, out.c_str(), out.size(), 0);
    if (sent > 0) {
        client.consumeOutput(static_cast<std::size_t>(sent));
        if (!client.hasOutput() && client.closeAfterWrite()) {
            removeClient(fd);
            return;
        }
        updatePollEvents(fd);
    } else if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return;
        std::cerr << "send failed fd=" << fd << ": " << strerror(errno) << std::endl;
        removeClient(fd);
    }
}

void Server::updatePollEvents(int fd)
{
    std::map<int, Client>::iterator cit = _clients.find(fd);
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
    _commands["PRIVMSG"] = &Command::handlePrivmsg;
    _commands["NOTICE"] = &Command::handleNotice;
    _commands["JOIN"] = &Command::handleJoin;
    _commands["PART"] = &Command::handlePart;
}

/* Komut dagitimi: komut adi (case-insensitive) tabloda aranir; bulunursa
   ilgili Command:: statik handler'i cagrilir. */
void Server::handleCommand(Client& client, const IRCMessage& msg)
{
    std::cout << "fd=" << client.getFd() << " > " << msg.command << std::endl;
    std::string cmd = Command::upper(msg.command);
    std::map<std::string, CmdFunc>::iterator it = _commands.find(cmd);
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

const std::string& Server::getPassword() const
{
    return _password;
}

std::map<int, Client>& Server::getClients()
{
    return _clients;
}

const std::map<int, Client>& Server::getClients() const
{
    return _clients;
}

std::string Server::getIsupport(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator it = _isupport.find(key);
    if (it == _isupport.end())
        return "";
    return it->second;
}

const std::string& Server::getCreationDate() const
{
    return _creationDate;
}

// Bir handler baska bir istemciye mesaj gonderdiginde, hedef fd icin
// POLLOUT'u hemen etkinlestirir. Boylece hedefin yeni bir komut yazmasini
// beklemeden tamponlu cikis poll dongusu tarafindan bosaltilir.
void Server::sendToClient(Client& client, const std::string& message)
{
    client.sendMessage(message);
    updatePollEvents(client.getFd());
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
    client.sendMessage(RPL_MYINFO(client.getName(TYPE_NICK), "ircserv 1.0", "i", "t,k,l"));
}

// Kanal adlari case-insensitive'dir: harita anahtari ircToLower ile normalize
// edilir; kanalin gorunen adi ise ilk olusturanin yazdigi haliyle Channel
// icinde saklanir.
Channel* Server::getChannel(const std::string& name)
{
    std::map<std::string, Channel>::iterator it = _channels.find(Client::ircToLower(name));
    if (it == _channels.end())
        return NULL;
    return &it->second;
}

// Channel'in default constructor'u yok; bu yuzden operator[] derlenmez,
// find + insert kullanilir.
Channel& Server::getOrCreateChannel(const std::string& name)
{
    std::string key = Client::ircToLower(name);
    std::map<std::string, Channel>::iterator it = _channels.find(key);
    if (it == _channels.end())
        it = _channels.insert(std::make_pair(key, Channel(name))).first;
    return it->second;
}

// Son uye ayrildiginda kanali kaldirir (PART/QUIT/KICK sonrasinda cagrilir).
void Server::removeEmptyChannel(const std::string& name)
{
    std::map<std::string, Channel>::iterator it = _channels.find(Client::ircToLower(name));
    if (it != _channels.end() && it->second.isEmpty())
        _channels.erase(it);
}

// Kanala yayin: Channel::broadcast mesaji uyelerin tamponina yazar; ardindan
// her uyenin fd'si icin POLLOUT etkinlestirilir ki mesajlar hedefler kendi
// olayini beklemeden aksin (sendToClient'in kanal karsiligi).
void Server::broadcastToChannel(const Channel& channel, const std::string& message, Client* except)
{
    channel.broadcast(message, except);
    const std::map<int, Client*>& members = channel.getMembers();
    for (std::map<int, Client*>::const_iterator it = members.begin(); it != members.end(); ++it)
        updatePollEvents(it->first);
}
