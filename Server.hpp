#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <vector>
# include <map>
# include <poll.h>

class Server
{
private:
    int                         _serverFd;
    int                         _port;
    std::string                 _password;
    std::vector<struct pollfd>  _pollFds;

    // Ağ mimarisi için bağımsız tamponlar (Aşama 1: Ağ Mimarisi)
    std::map<int, std::string>  _inBuffers;       // İstemci başına gelen veri tamponu
    std::map<int, std::string>  _outBuffers;      // İstemci başına giden veri tamponu
    std::map<int, bool>         _closeAfterWrite; // Yazma bittiğinde kapatma bayrağı

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

    // Komut ayrıştırma ve Aşama 1 kapsamındaki özel komutlar
    void extractAndHandleCommands(int fd);
    void handleCommand(int fd, const std::string& line);

    void handleCap(int fd, const std::vector<std::string>& tokens);
    void handlePing(int fd, const std::string& line);
    void handleQuit(int fd);

    // Ayrıştırma yardımcı fonksiyonları
    std::vector<std::string> split(const std::string& line) const;
    std::string upper(const std::string& str) const;
    std::string trimCr(const std::string& str) const;
};

#endif
