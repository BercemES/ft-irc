#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <iostream>
# include <vector>
# include <sys/socket.h>

# include "replies.hpp"

# define IRC_MAX_LINE 510

// Protocol line-length siniri degildir; yalniz yavas/okumayan bir alicinin
// server bellegini sinirsiz buyutmesini onleyen kaynak-koruma sinirdir.
# define MAX_PENDING_OUTPUT_BYTES (64 * 1024)

struct IRCMessage {
	std::string prefix;
	std::string command;
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
		int			_fd;
		std::string	_buffer;
		std::string	_nickname;
		std::string	_username;
		std::string	_realname;
		bool		_passFlag;
		bool		_nickFlag;
		bool		_userFlag;
		bool		_isRegistered;
		std::string	_host;
		std::string	_outBuffer;
		bool		_closeAfterWrite;
		bool		_outputOverflow;

		std::vector<IRCMessage>     _commandsOrder;

		IRCMessage			parseIRC(const std::string& line);
		static std::string	nextParam(const std::string& line, size_t& pos);
		
	public:
		Client(int fd);
		~Client();

		static std::string	ircToLower(std::string str);
		void				appendToBuffer(const std::string& message);
		bool				hasCommands() const;
		IRCMessage			getNextCommand();
		bool				isFullyRegistered() const;

		void				sendMessage(const std::string& message);
		bool				hasOutput() const;
		const std::string&	outBuffer() const;
		void				consumeOutput(std::size_t n);
		void				setCloseAfterWrite(bool value);
		bool				closeAfterWrite() const;
		bool				outputOverflow() const;

		std::string			getHost() const;
		void				setHost(const std::string& host);
		int					getFd() const;
		void				setName(NameType type, const std::string& value);
		std::string			getName(NameType type) const;
		void				setRegFlag(RegFlag flag, bool value);
		bool				getRegFlag(RegFlag flag) const;
};


#endif

