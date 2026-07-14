#include "channel.hpp"
#include "client.hpp"

Channel::Channel(const string& name)
	: _name(name)
{}

Channel::~Channel() {}

void	Channel::addMember(Client* client)
{
	if (client == NULL)
		return ;
	_members[client->getFd()] = client;
}

void	Channel::removeMember(Client* client)
{
	int	fd;

	if (client == NULL)
		return ;
	fd = client->getFd();
	_members.erase(fd);
	_operators.erase(fd);
	_invited.erase(fd);
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

const string	&Channel::getName() const
{
	return (_name);
}

const map<int, Client*>		&Channel::getMembers() const
{
	return (_members);
}
