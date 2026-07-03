#ifndef COMMAND_HPP
# define COMMAND_HPP

# include "server.hpp"
# include "client.hpp"

class Command {
public:

    static void	handlePass(server& server, Client& client, const IRCMessage& msg);
    static void	handleNick(server& server, Client& client, const IRCMessage& msg);
    static void	handleUser(server& server, Client& client, const IRCMessage& msg);
    static void	handleMotd(server& server, Client& client, const IRCMessage& msg);
    static void	handlePrivmsg(server& server, Client& client, const IRCMessage& msg);
	static void	handleNotice(server& server, Client& client, const IRCMessage& msg);
};

#endif