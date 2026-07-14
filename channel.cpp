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
	if (client == NULL)
		return ;
	_members.erase(client->getFd());
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

const string	&Channel::getName() const
{
	return (_name);
}

const map<int, Client*>		&Channel::getMembers() const
{
	return (_members);
}
