#ifndef REPLIES_HPP
# define REPLIES_HPP

#include <iostream>

using std::string;

class Replies {
public:

static string	ERR_NEEDMOREPARAMS(const string& client, const string& command)
{
    return (":ircserv 461 " + client + " " + command + " :Not enough parameters\r\n");
}

static string	ERR_ALREADYREGISTERED(const string& client)
{

	return(":ircserv 462 " + client + " :You may not reregister\r\n");
}

static string	ERR_PASSWDMISMATCH(const string& client)
{
	return (":ircserv 464 " + client + " :Password incorrect\r\n");
}

//ERR_NONICKNAMEGIVEN (431)  "<client> :No nickname given"
//ERR_ERRONEUSNICKNAME (432) "<client> <nick> :Erroneus nickname"
//ERR_NICKNAMEINUSE (433) "<client> <nick> :Nickname is already in use"
//ERR_NICKCOLLISION (436) "<client> <nick> :Nickname collision KILL from <user>@<host>"

};

#endif