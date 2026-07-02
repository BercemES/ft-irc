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

static string	ERR_NONICKNAMEGIVEN(const string& client)
{
	return (":ircserv 431 " + client + " :No nickname given\r\n");
}

static string	ERR_ERRONEUSNICKNAME(const string& client, const string& nick)
{
	return (":ircserv 432 " + client + " " + nick + " :Erroneus nickname\r\n");
}

static string	ERR_NICKNAMEINUSE(const string& client, const string& nick)
{
	return (":ircserv 433 " + client + " " + nick + " :Nickname is already in use\r\n");
}

//ERR_NICKCOLLISION (436) "<client> <nick> :Nickname collision KILL from <user>@<host>" -----İki sunucu bağlı olarak işlem yapmadığımız için ele almıyoruz.

};

#endif