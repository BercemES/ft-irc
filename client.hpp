#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <iostream>
# include <vector>
# include <sys/socket.h>

# include "replies.hpp"

# define IRC_MAX_LINE 510

using std::string;

struct IRCMessage {
	string prefix;
	string command;
	std::vector<std::string> params;
};

enum NameType {
    TYPE_NICK,
    TYPE_USER,
    TYPE_REAL
};

enum RegFlag {
	FLAG_PASS,
	FLAG_NICK,
	FLAG_USER,
	FLAG_REGISTERED
};

class Client
{
	private:
		int		_fd;
		string	_buffer;
		string	_nickname;
		string	_username;
		string	_realname;
		bool	_passFlag;
		bool	_nickFlag;
		bool	_userFlag;
		bool	_isRegistered;

		std::vector<IRCMessage>     _commandsOrder;

		IRCMessage		parseIRC(const std::string& line);
		static string	nextParam(const std::string& line, size_t& pos);
		
	public:
		Client(int fd);
		~Client();

		static string	ircToLower(string str);
		void			appendToBuffer(const std::string& message);
		bool			hasCommands() const;
		IRCMessage		getNextCommand();
		bool			isFullyRegistered() const;
		void			sendMessage(const std::string& message) const;

		int				getFd() const;
		void			setName(NameType type, const std::string& value);
		string			getName(NameType type) const;
		void			setRegFlag(RegFlag flag, bool value);
		bool			getRegFlag(RegFlag flag) const;
};


#endif

