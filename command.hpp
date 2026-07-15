#ifndef COMMAND_HPP
# define COMMAND_HPP

# include <string>

# include "client.hpp"   // Client, IRCMessage

class Server;

// Komut handler'lari ayni imzayi paylasir: (Server&, Client&, IRCMessage).
// Boylece Server'daki fonksiyon-pointer tablosuna (CmdFunc) tek tip olarak
// girerler. Yeni komutlar (PASS/NICK/USER/...) sonraki adimlarda eklenecek.
class Command
{
public:
    static void         handleCap(Server& server, Client& client, const IRCMessage& msg);
    static void         handlePing(Server& server, Client& client, const IRCMessage& msg);
    static void         handleQuit(Server& server, Client& client, const IRCMessage& msg);

    static void         handlePass(Server& server, Client& client, const IRCMessage& msg);
    static void         handleNick(Server& server, Client& client, const IRCMessage& msg);
    static void         handleUser(Server& server, Client& client, const IRCMessage& msg);

    static std::string  upper(const std::string& str);
};

#endif
