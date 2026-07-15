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

// NOT: ileride burada Client'in tum kanallardan cikarilmasi ve QUIT
// bildiriminin yayilmasi da yapilacak (kanal entegrasyonu adiminda).
void Server::removeClient(int fd)
{
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
        _clients.insert(std::make_pair(fd, Client(fd)));
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
}

/* Komut dagitimi: komut adi (case-insensitive) tabloda aranir; bulunursa
   ilgili Command:: statik handler'i cagrilir. Bilinmeyen komutlar simdilik
   yoksayilir (ERR_UNKNOWNCOMMAND kayit akisi geldiginde eklenecek). */
void Server::handleCommand(Client& client, const IRCMessage& msg)
{
    std::cout << "fd=" << client.getFd() << " > " << msg.command << std::endl;
    std::map<std::string, CmdFunc>::iterator it = _commands.find(Command::upper(msg.command));
    if (it == _commands.end())
    {
        std::cout << "Pass to other commands module: " << msg.command << std::endl;
        return;
    }
    it->second(*this, client, msg);
}