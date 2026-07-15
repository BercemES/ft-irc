#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <vector>
# include <map>
# include <poll.h>

# include "client.hpp"

class Server
{
private:
    int                         _serverFd;
    int                         _port;
    std::string                 _password;
    std::vector<struct pollfd>  _pollFds;

    // fd -> Client : her istemcinin oturumu, giris/cikis tamponu ve durumu
    // artik Client nesnesinde yasar (Asama 2: Client entegrasyonu).
    std::map<int, Client>       _clients;

public:
    Server(const std::string& port, const std::string& password);
    ~Server();

    void start();

private:
    Server();
    Server(const Server& other);
    Server& operator=(const Server& other);

    void setupSocket();
    void setNonBlocking(int fd);
    void addPollFd(int fd, short events);
    void removeClient(int fd);
    void acceptClients();
    void readFromClient(int fd);
    void writeToClient(int fd);
    void updatePollEvents(int fd);
    void handlePollEvents(size_t& i, int& ready);

    // Komut dagitimi (su an if-else; Adim 3'te komut tablosuna donusecek)
    void handleCommand(Client& client, const IRCMessage& msg);

    void handleCap(Client& client, const IRCMessage& msg);
    void handlePing(Client& client, const IRCMessage& msg);
    void handleQuit(Client& client);

    std::string upper(const std::string& str) const;
};

#endif
