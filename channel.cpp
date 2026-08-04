#include "channel.hpp"
#include "client.hpp"

#include <sstream>

using std::string;
using std::map;

Channel::Channel(const string& name)
	: _name(name), _topicTime(0), _userLimit(0), _inviteOnly(false),
	  _topicRestricted(false)
{}

Channel::~Channel() {}

void	Channel::addMember(Client* client)
{
	if (client == NULL)
		return ;
	_members[client->getFd()] = client;
	// Davet tek kullanimliktir: uye kanala girer girmez bekleyen davet tuketilir.
	_invited.erase(client->getFd());
}

void	Channel::removeMember(Client* client)
{
	int	fd;

	if (client == NULL)
		return ;
	fd = client->getFd();
	_members.erase(fd);
	_operators.erase(fd);
	// Davet durumu burada temizlenmez: uyelik ve davet ayri yasam dongulerine
	// sahiptir (bir davetli hicbir zaman uye olmayabilir). Bkz. removeInvite().
}

bool	Channel::isMember(Client* client) const
{
	if (client == NULL)
		return (false);
	return (_members.find(client->getFd()) != _members.end());
}

bool	Channel::isEmpty() const
{
	return (_members.empty());
}

size_t	Channel::size() const
{
	return (_members.size());
}

void	Channel::addOperator(Client* client)
{
	if (client == NULL)
		return ;
	if (_members.find(client->getFd()) != _members.end())
		_operators.insert(client->getFd());
}

void	Channel::removeOperator(Client* client)
{
	if (client == NULL)
		return ;
	_operators.erase(client->getFd());
}

bool	Channel::isOperator(Client* client) const
{
	if (client == NULL)
		return (false);
	return (_operators.find(client->getFd()) != _operators.end());
}

void	Channel::invite(Client* client)
{
	if (client == NULL)
		return ;
	_invited.insert(client->getFd());
}

bool	Channel::isInvited(Client* client) const
{
	if (client == NULL)
		return (false);
	return (_invited.find(client->getFd()) != _invited.end());
}

// Davet, uyelikten bagimsiz bir yasam dongusune sahiptir: davet edilen henuz
// uye olmayabilir. Bu yuzden temizligi removeMember()'a gizlice baglamiyoruz;
// disconnect sirasinda uyelikten bagimsiz olarak acikca cagrilir.
void	Channel::removeInvite(Client* client)
{
	if (client == NULL)
		return ;
	_invited.erase(client->getFd());
}

bool	Channel::hasTopic() const
{
	return (!_topic.empty());
}

void	Channel::setTopic(const string& topic, const string& setterNick)
{
	_topic = topic;
	_topicSetter = setterNick;
	_topicTime = std::time(NULL);
}

time_t	Channel::getTopicTime() const
{
	return (_topicTime);
}

const string	&Channel::getTopic() const
{
	return (_topic);
}

const string	&Channel::getTopicSetter() const
{
	return (_topicSetter);
}

bool	Channel::isInviteOnly() const
{
	return (_inviteOnly);
}

void	Channel::setInviteOnly(bool on)
{
	_inviteOnly = on;
}

bool	Channel::isTopicRestricted() const
{
	return (_topicRestricted);
}

void	Channel::setTopicRestricted(bool on)
{
	_topicRestricted = on;
}

bool	Channel::hasKey() const
{
	return (!_key.empty());
}

bool	Channel::checkKey(const string& key) const
{
	return (_key == key);
}

void	Channel::setKey(const string& key)
{
	_key = key;
}

void	Channel::clearKey()
{
	_key.clear();
}

bool	Channel::hasUserLimit() const
{
	return (_userLimit > 0);
}

bool	Channel::isFull() const
{
	if (_userLimit == 0)
		return (false);
	return (_members.size() >= _userLimit);
}

void	Channel::setUserLimit(size_t limit)
{
	_userLimit = limit;
}

void	Channel::clearUserLimit()
{
	_userLimit = 0;
}

string	Channel::getModeString() const
{
	std::ostringstream	oss;
	string				modes = "+";
	string				params;

	if (_inviteOnly)
		modes += "i";
	if (_topicRestricted)
		modes += "t";
	if (hasKey())
	{
		modes += "k";
		params += " " + _key;
	}
	if (hasUserLimit())
	{
		oss << _userLimit;
		modes += "l";
		params += " " + oss.str();
	}
	return (modes + params);
}

const string	&Channel::getName() const
{
	return (_name);
}

const map<int, Client*>		&Channel::getMembers() const
{
	return (_members);
}

void	Channel::broadcast(const string& message, Client* except) const
{
	map<int, Client*>::const_iterator	it;

	for (it = _members.begin(); it != _members.end(); ++it)
	{
		if (it->second == NULL)
			continue ;
		if (except != NULL && it->second == except)
			continue ;
		it->second->sendMessage(message);
	}
}
