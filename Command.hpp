#ifndef COMMAND_HPP
# define COMMAND_HPP

# include <string>

# include "Client.hpp"

class Server;
class Channel;

class Command
{
	private:
    	static bool         prepareChannelMode(Server& server, Client& client, const IRCMessage& msg, Channel*& chan);
	public:
	    static void         handleCap(Server& server, Client& client, const IRCMessage& msg);
	    static void         handlePing(Server& server, Client& client, const IRCMessage& msg);
	    static void         handleQuit(Server& server, Client& client, const IRCMessage& msg);

	    static void         handlePass(Server& server, Client& client, const IRCMessage& msg);
	    static void         handleNick(Server& server, Client& client, const IRCMessage& msg);
	    static void         handleUser(Server& server, Client& client, const IRCMessage& msg);

	    static void         handleMotd(Server& server, Client& client, const IRCMessage& msg);
	    static void         handleWho(Server& server, Client& client, const IRCMessage& msg);
	    static void         handlePrivmsg(Server& server, Client& client, const IRCMessage& msg);
	    static void         handleNotice(Server& server, Client& client, const IRCMessage& msg);

	    static void         handleJoin(Server& server, Client& client, const IRCMessage& msg);
	    static void         handlePart(Server& server, Client& client, const IRCMessage& msg);
	    static void         handleTopic(Server& server, Client& client, const IRCMessage& msg);
	    static void         handleMode(Server& server, Client& client, const IRCMessage& msg);
	    static void         handleKick(Server& server, Client& client, const IRCMessage& msg);
	    static void         handleInvite(Server& server, Client& client, const IRCMessage& msg);

	    static std::string  upper(const std::string& str);
};

#endif
