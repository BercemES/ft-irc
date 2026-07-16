#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <string>
# include <map>
# include <set>
# include <ctime>
# include <cstddef>

class Client;

class Channel
{
	private:
		std::string				_name;
		std::map<int, Client*>	_members;
		std::set<int>			_operators;
		std::set<int>			_invited;
		std::string				_topic;
		std::string				_topicSetter;
		time_t					_topicTime;
		std::string				_key;
		size_t					_userLimit;
		bool					_inviteOnly;
		bool					_topicRestricted;

	public:
		explicit Channel(const std::string& name);
		~Channel();

		void							addMember(Client* client);
		void							removeMember(Client* client);
		bool							isMember(Client* client) const;
		bool							isEmpty() const;
		size_t							size() const;

		void							addOperator(Client* client);
		void							removeOperator(Client* client);
		bool							isOperator(Client* client) const;

		void							invite(Client* client);
		bool							isInvited(Client* client) const;

		bool							hasTopic() const;
		void							setTopic(const std::string& topic, const std::string& setterNick);
		time_t							getTopicTime() const;
		const std::string				&getTopic() const;
		const std::string				&getTopicSetter() const;

		bool							isInviteOnly() const;
		void							setInviteOnly(bool on);
		bool							isTopicRestricted() const;
		void							setTopicRestricted(bool on);

		bool							hasKey() const;
		bool							checkKey(const std::string& key) const;
		void							setKey(const std::string& key);
		void							clearKey();

		bool							hasUserLimit() const;
		bool							isFull() const;
		void							setUserLimit(size_t limit);
		void							clearUserLimit();

		std::string						getModeString() const;

		void							broadcast(const std::string& message, Client* except = NULL) const;

		const std::string				&getName() const;
		const std::map<int, Client*>	&getMembers() const;
};

#endif
