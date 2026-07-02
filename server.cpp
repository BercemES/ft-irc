#include "server.hpp"

//void server::initCommands()
//{
//    _commands["PASS"] = &handleCap();
//    _commands["PASS"] = &handlePass();
//    _commands["NICK"] = &handleNick();
//    _commands["USER"] = &handleUser();
//	//...
//}

void server::handleCommand(Client& client, IRCMessage& msg)
{
	std::map<std::string, Acommand*>::iterator it;
    
	if (msg.command.empty())
        return ;
	it = _commands.find(msg.command);
	if (it == _commands.end())
    {
        cout << ("421 " + msg.command + " :Unknown command") << endl;
        return ;
    }
	if (!client.isFullyRegistered() 
		&& msg.command != "PASS" 
		&& msg.command != "NICK" 
		&& msg.command != "USER")
	{
		cout << ("451 :You have not registered") << endl;
		return ;
	}
	it->second->execute(*this, client, msg);
}

const string	server::getPassword() const 
{
	return (this->_password);
}

const std::map<int, Client> server::getClients() const
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

