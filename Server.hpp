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

    // Komut adi -> handler. if-else yerine fonksiyon-pointer tablosu.
    typedef void (*CmdFunc)(Server&, Client&, const IRCMessage&);
    std::map<std::string, CmdFunc>  _commands;

    std::map<std::string, std::string>  _isupport;      // RPL_ISUPPORT (005) tokenlari
    std::string                 _creationDate;          // RPL_CREATED (003) icin

public:
    Server(const std::string& port, const std::string& password);
    ~Server();

    void start();

    // Kayit (registration) altyapisi — eski `server` sinifindan devralindi.
    // Command:: handler'lari bu erisimcileri kullanir.
    const std::string&              getPassword() const;
    std::map<int, Client>&          getClients();
    const std::map<int, Client>&    getClients() const;
    std::string                     getIsupport(const std::string& key) const;
    const std::string&              getCreationDate() const;
    void                            checkReg(Client& client) const;
    void                            sendToClient(Client& client, const std::string& message);

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

    // Komut dagitimi: fonksiyon-pointer tablosu (Command:: statik handler'lari)
    void initCommands();
    void initIsupport();
    void handleCommand(Client& client, const IRCMessage& msg);
};

#endif
