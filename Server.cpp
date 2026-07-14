#include "Server.hpp"

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

/* TODO: İstemci (Client) modülü bağlantı kesilme entegrasyonu:
   - Client oturumu, takma adı (nickname) ve kayıt bilgileri silinir.
   - Client tüm kanallardan çıkarılır ve diğer kullanıcılara bildirim (QUIT/PART) gönderilir.
   Örnek: clientManager.removeClient(fd);
*/
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
    _inBuffers.erase(fd);
    _outBuffers.erase(fd);
    _closeAfterWrite.erase(fd);
}

/* TODO: Yeni bağlantı kabul edildiğinde İstemci (Client) modülü entegrasyonu:
   Geliştirilen Client sınıfından yeni bir nesne üretilip sunucuya bağlanmalıdır.
   Örnek: clientManager.addClient(fd, inet_ntoa(addr.sin_addr));
*/
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
        _inBuffers[fd] = "";
        _outBuffers[fd] = "";
        _closeAfterWrite[fd] = false;
        std::cout << "New client fd=" << fd << " ip=" << inet_ntoa(addr.sin_addr) << std::endl;
    }
}

/* TODO: İstemci (Client) modülü veri işleme entegrasyonu:
   Gelen ham veri (buffer), ilgili istemcinin giriş kuyruğuna (inBuffer) eklenmelidir.
   Örnek:
   Client& client = clientManager.getClient(fd);
   client.appendIn(std::string(buffer, bytes));
   clientManager.processCommands(client);
*/
void Server::readFromClient(int fd)
{
    char buffer[512];
    while (true) {
        std::memset(buffer, 0, sizeof(buffer));
        ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
        if (bytes > 0) {
            _inBuffers[fd].append(buffer, bytes);
            extractAndHandleCommands(fd);
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
}

/* TODO: İstemci (Client) modülü yazma entegrasyonu:
   Bu alanda istemci modülündeki çıkış kuyruğundan (outBuffer) veri alınıp gönderilecektir.
   Örnek:
   Client& client = clientManager.getClient(fd);
   std::string& out = client.outBuffer();
   // send(fd, out.c_str(), out.size(), 0) ...
*/
void Server::writeToClient(int fd)
{
    std::string& out = _outBuffers[fd];
    if (out.empty()) {
        updatePollEvents(fd);
        if (_closeAfterWrite[fd]) removeClient(fd);
        return;
    }
    ssize_t sent = send(fd, out.c_str(), out.size(), 0);
    if (sent > 0) {
        out.erase(0, sent);
        if (out.empty() && _closeAfterWrite[fd]) {
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
    for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
    {
        if (it->fd == fd)
        {
            it->events = POLLIN;
            if (!_outBuffers[fd].empty())
                it->events |= POLLOUT;
            break;
        }
    }
}

void Server::extractAndHandleCommands(int fd)
{
    std::string& buf = _inBuffers[fd];
    size_t pos;
    while ((pos = buf.find('\n')) != std::string::npos)
    {
        std::string line = buf.substr(0, pos);
        buf.erase(0, pos + 1);
        line = trimCr(line);
        if (!line.empty())
            handleCommand(fd, line);
        if (_inBuffers.find(fd) == _inBuffers.end())
            return;
    }
}

/* TODO: NICK, USER, JOIN, PRIVMSG vb. diğer komut modülleri
   buraya entegre edilecektir.
   Örnek: clientManager.handle(fd, tokens);
*/
void Server::handleCommand(int fd, const std::string& line)
{
    std::vector<std::string> tokens = split(line);
    if (tokens.empty()) return;
    std::string cmd = upper(tokens[0]);
    std::cout << "fd=" << fd << " > " << line << std::endl;
    if (cmd == "CAP")
        handleCap(fd, tokens);
    else if (cmd == "PING")
        handlePing(fd, line);
    else if (cmd == "QUIT")
        handleQuit(fd);
    else
        std::cout << "Pass to other commands module: " << line << std::endl;
    if (_inBuffers.find(fd) != _inBuffers.end())
        updatePollEvents(fd);
}

void Server::handleCap(int fd, const std::vector<std::string>& tokens)
{
    if (tokens.size() >= 2 && upper(tokens[1]) == "LS")
        _outBuffers[fd].append(":ircserv CAP * LS :\r\n");
    else if (tokens.size() >= 2 && upper(tokens[1]) == "END")
        return;
    else if (tokens.size() >= 2 && upper(tokens[1]) == "REQ")
        _outBuffers[fd].append(":ircserv CAP * NAK :\r\n");
    else
        _outBuffers[fd].append(":ircserv CAP * LS :\r\n");
}

void Server::handlePing(int fd, const std::string& line)
{
    std::string payload;
    size_t pos = line.find(' ');
    if (pos != std::string::npos)
        payload = line.substr(pos + 1);
    if (payload.empty())
        payload = ":ircserv";
    _outBuffers[fd].append("PONG " + payload + "\r\n");
}

void Server::handleQuit(int fd)
{
    _outBuffers[fd].append("ERROR :Closing Link\r\n");
    _closeAfterWrite[fd] = true;
}

std::vector<std::string> Server::split(const std::string& line) const
{
    std::vector<std::string> result;
    std::istringstream iss(line);
    std::string item;
    while (iss >> item)
        result.push_back(item);
    return result;
}

std::string Server::upper(const std::string& str) const
{
    std::string out = str;
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[i])));
    return out;
}

std::string Server::trimCr(const std::string& str) const
{
    if (!str.empty() && str[str.size() - 1] == '\r')
        return str.substr(0, str.size() - 1);
    return str;
}