#ifndef REPLIES_HPP
# define REPLIES_HPP

# include <string>

//registration
inline std::string   RPL_WELCOME(const std::string& nick, const std::string& user, const std::string& host)
{
    return (":ircserv 001 " + nick + " :Welcome to the IRC Network, " + nick + "!" + user + "@" + host + "\r\n");
}

inline std::string   RPL_YOURHOST(const std::string& nick, const std::string& version)
{
    return (":ircserv 002 " + nick + " :Your host is ircserv, running version " + version + "\r\n");
}

inline std::string   RPL_CREATED(const std::string& nick, const std::string& datetime)
{
    return (":ircserv 003 " + nick + " :This server was created " + datetime + "\r\n");
}

inline std::string   RPL_MYINFO(const std::string& nick, const std::string& version, const std::string& umodes, const std::string& cmodes)
{
    return (":ircserv 004 " + nick + " " + "ircserv" + " " + version + " " + umodes + " " + cmodes + "\r\n");
}

inline std::string   RPL_ISUPPORT(const std::string& nick, const std::string& tokens)
{
    return (":ircserv 005 " + nick + " " + tokens + " :are supported by this server\r\n");
}

//RPL
inline std::string	RPL_MOTDSTART(const std::string& client)
{
	return (":ircserv 375 " + client + " :- ircserv Message of the day -\r\n");
}

inline std::string	RPL_MOTD(const std::string& client, const std::string& lineOfTheMotd)
{
	return (":ircserv 372 " + client + " :" + lineOfTheMotd + "\r\n");
}

inline std::string	RPL_ENDOFMOTD(const std::string& client)
{
	return (":ircserv 376 " + client + " :End of /MOTD command.\r\n");
}

//error

inline std::string	ERR_NOSUCHNICK(const std::string& client, const std::string& nickname)
{
	return (":ircserv 401 " + client + " " + nickname + " :No such nick/channel\r\n");
}

inline std::string	ERR_NOSUCHSERVER(const std::string& client, const std::string& serverName)
{
	return (":ircserv 402 " + client + " " + serverName + " :No such server\r\n");
}

inline std::string	ERR_CANNOTSENDTOCHAN(const std::string& client, const std::string& channel)
{
	return (":ircserv 404 " + client + channel + " :Cannot send to channel\r\n");
}

inline std::string	ERR_NORECIPIENT(const std::string& client, const std::string& command)
{
	return (":ircserv 411 " + client + " :No recipient given (" + command + ")\r\n");
}

inline std::string	ERR_NOTEXTTOSEND(const std::string& client)
{
	return (":ircserv 412 " + client + " :No text to send\r\n");
}

inline std::string	ERR_INPUTTOOLONG(const std::string& client)
{
	return (":ircserv 417 " + client + " :Input line was too long\r\n");
}

inline std::string	ERR_UNKNOWNCOMMAND(const std::string& client, const std::string& command)
{
    return (":ircserv 421 " + client + " " + command + " :Unknown command\r\n");
}

inline std::string	ERR_NOTREGISTERED(const std::string& client)
{
    return (":ircserv 451 " + client + " :You have not registered\r\n");
}

inline std::string	ERR_NEEDMOREPARAMS(const std::string& client, const std::string& command)
{
    return (":ircserv 461 " + client + " " + command + " :Not enough parameters\r\n");
}

inline std::string	ERR_ALREADYREGISTERED(const std::string& client)
{

	return(":ircserv 462 " + client + " :You may not reregister\r\n");
}

inline std::string	ERR_PASSWDMISMATCH(const std::string& client)
{
	return (":ircserv 464 " + client + " :Password incorrect\r\n");
}

inline std::string	ERR_NONICKNAMEGIVEN(const std::string& client)
{
	return (":ircserv 431 " + client + " :No nickname given\r\n");
}

inline std::string	ERR_ERRONEUSNICKNAME(const std::string& client, const std::string& nick)
{
	return (":ircserv 432 " + client + " " + nick + " :Erroneus nickname\r\n");
}

inline std::string	ERR_NICKNAMEINUSE(const std::string& client, const std::string& nick)
{
	return (":ircserv 433 " + client + " " + nick + " :Nickname is already in use\r\n");
}

//ERR_NICKCOLLISION (436) "<client> <nick> :Nickname collision KILL from <user>@<host>" -----İki sunucu bağlı olarak işlem yapmadığımız için ele almıyoruz.

//channel

inline std::string	RPL_NOTOPIC(const std::string& nick, const std::string& channel)
{
	return (":ircserv 331 " + nick + " " + channel + " :No topic is set\r\n");
}

inline std::string	RPL_TOPIC(const std::string& nick, const std::string& channel, const std::string& topic)
{
	return (":ircserv 332 " + nick + " " + channel + " :" + topic + "\r\n");
}

inline std::string	RPL_TOPICWHOTIME(const std::string& nick, const std::string& channel, const std::string& setter, const std::string& setat)
{
	return (":ircserv 333 " + nick + " " + channel + " " + setter + " " + setat + "\r\n");
}

inline std::string	RPL_NAMREPLY(const std::string& nick, const std::string& channel, const std::string& names)
{
	return (":ircserv 353 " + nick + " = " + channel + " :" + names + "\r\n");
}

inline std::string	RPL_ENDOFNAMES(const std::string& nick, const std::string& channel)
{
	return (":ircserv 366 " + nick + " " + channel + " :End of /NAMES list\r\n");
}

inline std::string	ERR_NOSUCHCHANNEL(const std::string& client, const std::string& channel)
{
	return (":ircserv 403 " + client + " " + channel + " :No such channel\r\n");
}

inline std::string	ERR_CHANNELISFULL(const std::string& client, const std::string& channel)
{
	return (":ircserv 471 " + client + " " + channel + " :Cannot join channel (+l)\r\n");
}

inline std::string	ERR_INVITEONLYCHAN(const std::string& client, const std::string& channel)
{
	return (":ircserv 473 " + client + " " + channel + " :Cannot join channel (+i)\r\n");
}

inline std::string	ERR_BADCHANNELKEY(const std::string& client, const std::string& channel)
{
	return (":ircserv 475 " + client + " " + channel + " :Cannot join channel (+k)\r\n");
}

inline std::string	ERR_BADCHANMASK(const std::string& client, const std::string& channel)
{
	return (":ircserv 476 " + client + " " + channel + " :Bad Channel Mask\r\n");
}

#endif
