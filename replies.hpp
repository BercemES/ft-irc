#ifndef REPLIES_HPP
# define REPLIES_HPP

# include <string>

using std::string;

//registration
inline string   RPL_WELCOME(const string& nick, const string& user, const string& host)
{
    return (":ircserv 001 " + nick + " :Welcome to the IRC Network, " + nick + "!" + user + "@" + host + "\r\n");
}

inline string   RPL_YOURHOST(const string& nick, const string& version)
{
    return (":ircserv 002 " + nick + " :Your host is ircserv" + ", running version " + version + "\r\n");
}

inline string   RPL_CREATED(const string& nick, const string& datetime)
{
    return (":ircserv 003 " + nick + " :This server was created " + datetime + "\r\n");
}

inline string   RPL_MYINFO(const string& nick, const string& version, const string& umodes, const string& cmodes)
{
    return (":ircserv 004 " + nick + " " + "ircserv" + " " + version + " " + umodes + " " + cmodes + "\r\n");
}

inline string   RPL_ISUPPORT(const string& nick, const string& tokens)
{
    return ("::ircserv 005 " + nick + " " + tokens + " :are supported by this server\r\n");
}

//error

inline string	ERR_INPUTTOOLONG(const string& client)
{
	return (":ircserv 417 " + client + " :Input line was too long\r\n");
}

inline string	ERR_UNKNOWNCOMMAND(const string& client, const string& command)
{
    return (":ircserv 421 " + client + " " + command + " :Unknown command\r\n");
}

inline string	ERR_NOTREGISTERED(const string& client)
{
    return (":ircserv 451 " + client + " :You have not registered\r\n");
}

inline string	ERR_NEEDMOREPARAMS(const string& client, const string& command)
{
    return (":ircserv 461 " + client + " " + command + " :Not enough parameters\r\n");
}

inline string	ERR_ALREADYREGISTERED(const string& client)
{

	return(":ircserv 462 " + client + " :You may not reregister\r\n");
}

inline string	ERR_PASSWDMISMATCH(const string& client)
{
	return (":ircserv 464 " + client + " :Password incorrect\r\n");
}

inline string	ERR_NONICKNAMEGIVEN(const string& client)
{
	return (":ircserv 431 " + client + " :No nickname given\r\n");
}

inline string	ERR_ERRONEUSNICKNAME(const string& client, const string& nick)
{
	return (":ircserv 432 " + client + " " + nick + " :Erroneus nickname\r\n");
}

inline string	ERR_NICKNAMEINUSE(const string& client, const string& nick)
{
	return (":ircserv 433 " + client + " " + nick + " :Nickname is already in use\r\n");
}

//ERR_NICKCOLLISION (436) "<client> <nick> :Nickname collision KILL from <user>@<host>" -----İki sunucu bağlı olarak işlem yapmadığımız için ele almıyoruz.

//channel

inline string	RPL_NOTOPIC(const string& nick, const string& channel)
{
	return (":ircserv 331 " + nick + " " + channel + " :No topic is set\r\n");
}

inline string	RPL_TOPIC(const string& nick, const string& channel, const string& topic)
{
	return (":ircserv 332 " + nick + " " + channel + " :" + topic + "\r\n");
}

inline string	RPL_TOPICWHOTIME(const string& nick, const string& channel, const string& setter, const string& setat)
{
	return (":ircserv 333 " + nick + " " + channel + " " + setter + " " + setat + "\r\n");
}

inline string	RPL_NAMREPLY(const string& nick, const string& channel, const string& names)
{
	return (":ircserv 353 " + nick + " = " + channel + " :" + names + "\r\n");
}

inline string	RPL_ENDOFNAMES(const string& nick, const string& channel)
{
	return (":ircserv 366 " + nick + " " + channel + " :End of /NAMES list\r\n");
}

inline string	ERR_NOSUCHCHANNEL(const string& client, const string& channel)
{
	return (":ircserv 403 " + client + " " + channel + " :No such channel\r\n");
}

inline string	ERR_CHANNELISFULL(const string& client, const string& channel)
{
	return (":ircserv 471 " + client + " " + channel + " :Cannot join channel (+l)\r\n");
}

inline string	ERR_INVITEONLYCHAN(const string& client, const string& channel)
{
	return (":ircserv 473 " + client + " " + channel + " :Cannot join channel (+i)\r\n");
}

inline string	ERR_BADCHANNELKEY(const string& client, const string& channel)
{
	return (":ircserv 475 " + client + " " + channel + " :Cannot join channel (+k)\r\n");
}

inline string	ERR_BADCHANMASK(const string& client, const string& channel)
{
	return (":ircserv 476 " + client + " " + channel + " :Bad Channel Mask\r\n");
}

#endif