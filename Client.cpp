#include "Client.hpp"

using std::string;

Client::Client(int fd):
	_fd(fd), _passFlag(false), _nickFlag(false), _userFlag(false),
	_isRegistered(false), _closeAfterWrite(false), _outputOverflow(false),
	_quitReason("Client Quit") {}

Client::~Client() {}

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

	if (this->_closeAfterWrite)
		return ;
	this->_buffer += message;
	pos = _buffer.find("\r\n");
	while (pos != std::string::npos)
	{
		line = _buffer.substr(0, pos);
		_buffer.erase(0, pos + 2);
		if (line.size() > IRC_MAX_LINE)
			this->sendMessage(ERR_INPUTTOOLONG(this->getName(TYPE_NICK)));
		else
		{
			msg = parseIRC(line);
			if (!msg.command.empty())
				this->_commandsOrder.push_back(msg);
		}
		pos = _buffer.find("\r\n");
	}
	if (_buffer.size() > IRC_MAX_LINE)
	{

		this->sendMessage(ERR_INPUTTOOLONG(this->getName(TYPE_NICK)));
		this->_buffer.clear();
		this->setCloseAfterWrite(true);
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

string	Client::getHost() const
{
	return (this->_host);
}

void	Client::setHost(const string& host)
{
	this->_host = host;
}

int	Client::getFd() const
{
	return (this->_fd);
}

string	Client::getName(NameType type) const
{
	if (type == TYPE_NICK)
	{
		if (this->_nickname.empty())
			return("*");

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

void	Client::sendMessage(const string& message)
{
	if (this->_outputOverflow)
		return ;
	if (this->_outBuffer.size() + message.size() > MAX_PENDING_OUTPUT_BYTES)
	{

		this->_outputOverflow = true;
		return ;
	}
	this->_outBuffer += message;
}

bool	Client::hasOutput() const
{
	return (!this->_outBuffer.empty());
}

const string&	Client::outBuffer() const
{
	return (this->_outBuffer);
}

void	Client::consumeOutput(std::size_t n)
{
	this->_outBuffer.erase(0, n);
}

void	Client::setCloseAfterWrite(bool value)
{
	this->_closeAfterWrite = value;
}

bool	Client::closeAfterWrite() const
{
	return (this->_closeAfterWrite);
}

bool	Client::outputOverflow() const
{
	return (this->_outputOverflow);
}

void	Client::setRegFlag(RegFlag flag, bool value)
{
	if (flag == FLAG_PASS)
		this->_passFlag = value;
	else if (flag == FLAG_NICK)
		this->_nickFlag = value;
	else if (flag == FLAG_USER)
		this->_userFlag = value;
	else if (flag == FLAG_REGISTERED)
        this->_isRegistered = value;
}

bool	Client::getRegFlag(RegFlag flag) const
{
	if (flag == FLAG_PASS)
		return (this->_passFlag);
	else if (flag == FLAG_NICK)
		return (this->_nickFlag);
	else if (flag == FLAG_USER)
		return (this->_userFlag);
	else if (flag == FLAG_REGISTERED)
        return (this->_isRegistered);
	return (false);
}

string Client::getFullPrefix() const
{
	return (":" + this->getName(TYPE_NICK) + "!"
		+ this->getName(TYPE_USER) + "@" + this->getHost());
}

void	Client::setQuitReason(const string& reason)
{
	this->_quitReason = reason;
}

string	Client::getQuitReason() const
{
	return (this->_quitReason);
}
