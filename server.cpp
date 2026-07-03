#include "server.hpp"

void	server::initCommands()
{
	_commands["PASS"] = &Command::handlePass;
	_commands["NICK"] = &Command::handleNick;
	_commands["USER"] = &Command::handleUser;
	_commands["MOTD"] = &Command::handleMotd;
	_commands["PRIVMSG"]= &Command::handlePrivmsg;
	_commands["NOTICE"] = &Command::handleNotice;
	//...
}

void	server::handleCommand(Client& client, IRCMessage& msg)
{
	if (msg.command.empty())
		return ;
	std::map<std::string, CmdFunc>::iterator it = _commands.find(msg.command);
	if (it == _commands.end())
	{
		client.sendMessage(ERR_UNKNOWNCOMMAND(client.getName(TYPE_NICK), msg.command));
		return ;
	}
	if (!client.isFullyRegistered() 
		&& msg.command != "PASS" 
		&& msg.command != "NICK" 
		&& msg.command != "USER")
	{
		client.sendMessage(ERR_NOTREGISTERED(client.getName(TYPE_NICK)));
		return ;
	}
	it->second(*this, client, msg);
	
}

bool	server::checkReg(Client& client) const
{
	if (client.getRegFlag(FLAG_REGISTERED))
		return;
	if (client.isFullyRegistered())
	{
		client.setRegFlag(FLAG_REGISTERED, true);
		client.sendMessage(RPL_WELCOME(client.getName(TYPE_NICK), client.getName(TYPE_USER), "localhost"));
		client.sendMessage(RPL_YOURHOST(client.getName(TYPE_NICK), "ircserv 1.0"));
		client.sendMessage(RPL_CREATED(client.getName(TYPE_NICK), this->getCreationDate()));
		client.sendMessage(RPL_MYINFO(client.getName(TYPE_NICK), "ircserv 1.0", "i", "t,k,l"));
	}
	/*
	RPL_ISUPPORT (005) "<client> <1-13 tokens> :are supported by this server"
	*/
}

const string	server::getPassword() const 
{
	return (this->_password);
}

const std::map<int, Client>& server::getClients() const
{
	return (this->_clients);
}

void server::initIsupport()
{
	
	this->_RPL_ISUPPORT["USERLEN"] = "15";
	/*
	token      =  *1"-" parameter / parameter *1( "=" value )
  parameter  =  1*20 (letter / "." / "/")
  value      =  * letpun
  letter     =  ALPHA / DIGIT
  punct      =  %d33-47 / %d58-64 / %d91-96 / %d123-126
  letpun     =  letter / punct
	*/
}

const string server::getIsupport(const std::string& key) const
{
	std::map<std::string, std::string>::const_iterator it;
	it = this->_RPL_ISUPPORT.find(key);
	return (it->second);
}

