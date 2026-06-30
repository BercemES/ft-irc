#ifndef REPLIES_HPP
# define REPLIES_HPP

#include <iostream>

using std::string;

class Replies {
public:

string	errNeedMoreParams (const string& Client, const string& Command);

ERR_NEEDMOREPARAMS (461) "<client> <command> :Not enough parameters"
ERR_ALREADYREGISTERED (462) "<client> :You may not reregister"
ERR_PASSWDMISMATCH (464) "<client> :Password incorrect"
ERR_NONICKNAMEGIVEN (431)  "<client> :No nickname given"
ERR_ERRONEUSNICKNAME (432) "<client> <nick> :Erroneus nickname"
ERR_NICKNAMEINUSE (433) "<client> <nick> :Nickname is already in use"
ERR_NICKCOLLISION (436) "<client> <nick> :Nickname collision KILL from <user>@<host>"
};

#endif