#include "server.hpp"

void handleCap(server& server, Client& client, const IRCMessage& msg)
{
	
	
}

void handlePass(server& server, Client& client, const IRCMessage& msg)
{
	if (msg.params.empty())
	{
		client.sendMessage(Replies::ERR_NEEDMOREPARAMS(client.getName(TYPE_NICK), msg.command));
		return ;
	}	
	if (client.isFullyRegistered())
	{
		client.sendMessage(Replies::ERR_ALREADYREGISTERED(client.getName(TYPE_NICK)));
		return ;
	}
	if (msg.params[0] != server.getPassword())
		client.sendMessage(Replies::ERR_PASSWDMISMATCH(client.getName(TYPE_NICK)));
	else
		client.setRegFlag(FLAG_PASS, true);
}

void handleNick(server& server, Client& client, const IRCMessage& msg)
{
	
}

void handleUser(server& server, Client& client, const IRCMessage& msg)
{
	
}



