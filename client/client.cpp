#include "client.hpp"

Client::Client(int fd): _fd(fd), _passFlag(false), _nickFlag(false), _userFlag(false) 	{}

Client::~Client(){}


string	nextParam(const string& line, size_t& pos)
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
		pos = _buffer.find("\r\n");
	}
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

bool	Client::isFullyRegistered() const
{
	
}
