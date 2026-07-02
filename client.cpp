#include "client.hpp"

Client::Client(int fd): _fd(fd), _passFlag(false), _nickFlag(false), _userFlag(false) {}

Client::~Client(){}


string	Client::nextParam(const string& line, size_t& pos)
{
	size_t	space = 0;
	string	param;

	if (pos == string::npos || pos >= line.size())
		return ("");
	space = line.find(' ', pos);
	if (space == string::npos)
	{
		param = line.substr(pos);
		pos = string::npos;
	}
	else
	{
		param = line.substr(pos, space - pos);
		pos = line.find_first_not_of(" ", space);
	}
	return (param);
}

IRCMessage	Client::parseIRC(const string& line)
{
	size_t		pos = 0;
	size_t		space = 0;
	IRCMessage	msg;

	pos = line.find_first_not_of(" ");
	if (pos == string::npos)
		return (msg);
	if (line[pos] == ':')
	{
		space = line.find(' ', pos);
		if (space == string::npos)
			return (msg);
		msg.prefix = line.substr(pos + 1, space - (pos + 1));
		pos = line.find_first_not_of(" ", space);
	}
	msg.command = nextParam(line, pos);
	if (msg.command.empty())
		return msg;
	while (pos != string::npos)
	{
		if (line[pos] == ':')
		{
			msg.params.push_back(line.substr(pos + 1));
			break ;
		}
		string param = nextParam(line, pos);
		if (param.empty())
			break ;
		msg.params.push_back(param);
	}
	return (msg);
}

void	Client::appendToBuffer(const string& message)
{
	string				line;
	size_t				pos;
	struct	IRCMessage	msg;

	this->_buffer += message;
	pos = _buffer.find("\r\n");
	while (pos != std::string::npos)
	{
		line = _buffer.substr(0, pos);
		_buffer.erase(0, pos + 2);
		msg = parseIRC(line);
		if (!msg.command.empty())
			this->_commandsOrder.push_back(msg);
		pos = _buffer.find("\r\n");
	}
}

bool Client::hasCommands() const
{
	return (!this->_commandsOrder.empty());
}

string	Client::ircToLower(string str)
{
	for (size_t i = 0; i < str.size(); i++)
	{
		if (str[i] >= 'A' && str[i] <= 'Z')
			str[i] += 32;
		else if (str[i] == '[')
			str[i] = '{';
		else if (str[i] == ']')
			str[i] = '}';
		else if (str[i] == '\\')
			str[i] = '|';
		else if (str[i] == '^')
			str[i] = '~';
	}
	return (str);
}

IRCMessage Client::getNextCommand()
{
    IRCMessage msg = this->_commandsOrder.front();
    this->_commandsOrder.erase(this->_commandsOrder.begin());
    return (msg);
}

bool	Client::isFullyRegistered() const
{
	return (this->_nickFlag && this->_passFlag && this->_userFlag);
}

bool	Client::checkReg() const
{
	if (!this->isFullyRegistered())
		return (false);
	/*
	RPL_WELCOME (001) "<client> :Welcome to the <networkname> Network, <nick>[!<user>@<host>]"
	RPL_YOURHOST (002) "<client> :Your host is <servername>, running version <version>"
	RPL_CREATED (003) "<client> :This server was created <datetime>"
	RPL_MYINFO (004) "<client> <servername> <version> <available user modes>
  <available channel modes> [<channel modes with a parameter>]"
  RPL_ISUPPORT (005) "<client> <1-13 tokens> :are supported by this server"
	*/
}


string	Client::getName(NameType type) const
{
	if (type == TYPE_NICK)
	{
		if (this->_nickname.empty())
			return("*"); 
// Error dönerken hatada nickname yoksa * gösterilmesi gerekiyor diye direkt gete ekledim.
// Kullanıcının nicki var mı diye kontrol etmek isterseniz empty() değil; getName(TYPE_NICK) != "*" a şeklinde kontrol etmemizz lazım  
		return (this->_nickname);
	}
	if (type == TYPE_USER)
		return (this->_username);
	if (type == TYPE_REAL)
		return (this->_realname);
	return ("");
}


void	Client::setName(NameType type, const std::string& value)
{
	if (type == TYPE_NICK)
	{
			this->_nickname = value;
			this->_nickFlag = true;
	}
	else if (type == TYPE_USER)
	{
		this->_username = value;
		this->_userFlag = true;
	}
	else if (type == TYPE_REAL)
		this->_realname = value;
}

int	Client::getFd() const
{
	return (this->_fd);
}


void	Client::sendMessage(const string& message)
{
	int send_flag;
	
	send_flag = send(this->getFd(), message.c_str(), message.length(), MSG_NOSIGNAL);
	if (send_flag == -1)
	{
		//kullanıcı düştü. Bağlantıyı kapat fonksiyonu gelecek.!!!
	}
}

void	Client::setRegFlag(RegFlag flag, bool value)
{
	if (flag == FLAG_PASS) 
		this->_passFlag = value;
	else if (flag == FLAG_NICK) 
		this->_nickFlag = value;
	else if (flag == FLAG_USER)
		this->_userFlag = value;
}

bool	Client::getRegFlag(RegFlag flag) const
{
	if (flag == FLAG_PASS) 
		return (this->_passFlag);
	else if (flag == FLAG_NICK) 
		return (this->_nickFlag);
	else if (flag == FLAG_USER)
		return (this->_userFlag);
}
