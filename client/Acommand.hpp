#ifndef ACOMMAND_HPP
# define ACOMMAND_HPP

# include "client.hpp"

class server;

class Acommand {
public:
	virtual ~Acommand() {}
	virtual void execute(server& server, Client& client, const IRCMessage& msg) = 0;
};

#endif