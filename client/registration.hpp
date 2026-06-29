#ifndef REGISTRATION_HPP
# define REGISTRATION_HPP

# include "Acommand.hpp"

class Pass : public Acommand {
public:
    Pass();
    void execute(server& server, Client& client, const IRCMessage& msg);
};

class Nick : public Acommand {
public:
    Nick();
    void execute(server& server, Client& client, const IRCMessage& msg);
};

class User : public Acommand {
public:
    User();
    void execute(server& server, Client& client, const IRCMessage& msg);
};

#endif