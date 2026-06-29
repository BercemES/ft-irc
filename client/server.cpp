#include "server.hpp"

void server::initCommands()
{
    _commands["PASS"] = new Pass();
    _commands["NICK"] = new Nick();
    _commands["USER"] = new User();
	//...
}

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
