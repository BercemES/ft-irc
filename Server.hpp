#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <vector>
# include <map>
# include <set>
# include <poll.h>

# include "client.hpp"
# include "channel.hpp"

class Server
{
private:
    int                         _serverFd;
    int                         _port;
    std::string                 _password;
    std::vector<struct pollfd>  _pollFds;

    std::map<int, Client>       _clients;

    std::map<std::string, Channel>  _channels;

    typedef void (*CmdFunc)(Server&, Client&, const IRCMessage&);
    std::map<std::string, CmdFunc>  _commands;

    std::map<std::string, std::string>  _isupport;
    std::string                 _creationDate;

    std::set<int>                _pendingRemovals;

public:
    Server(const std::string& port, const std::string& password);
    ~Server();

    void start();

    const std::string&              getPassword() const;
    std::map<int, Client>&          getClients();
    const std::map<int, Client>&    getClients() const;
    std::string                     getIsupport(const std::string& key) const;
    const std::string&              getCreationDate() const;
    void                            checkReg(Client& client) const;
    void                            sendToClient(Client& client, const std::string& message);
    Client*                         getClientByNick(const std::string& nick);

    Channel*                        getChannel(const std::string& name);
    Channel&                        getOrCreateChannel(const std::string& name);
    void                            removeEmptyChannel(const std::string& name);
    void                            broadcastToChannel(const Channel& channel,
                                        const std::string& message, Client* except = NULL);
    void                            broadcastNickChange(Client& client, const std::string& oldPrefix);

private:
    Server();
    Server(const Server& other);
    Server& operator=(const Server& other);

    void setupSocket();
    void setNonBlocking(int fd);
    void addPollFd(int fd, short events);
    void removeClient(int fd);
    void requestRemoval(int fd);
    void processPendingRemovals();
    void removeClientFromChannels(Client& client);
    void acceptClients();
    void readFromClient(int fd);
    void writeToClient(int fd);
    void updatePollEvents(int fd);
    void handlePollEvents(size_t& i, int& ready);

    void initCommands();
    void initIsupport();
    void handleCommand(Client& client, const IRCMessage& msg);
};

#endif
