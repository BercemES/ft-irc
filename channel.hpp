#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <string>
# include <map>
# include <set>
# include <ctime>
# include <cstddef>

class Client;

using std::string;
using std::map;
using std::set;

class Channel
{
	private:
		string				_name;
		map<int, Client*>	_members;
		set<int>			_operators;
		set<int>			_invited;
		string				_topic;
		string				_topicSetter;
		time_t				_topicTime;
		string				_key;
		size_t				_userLimit;
		bool				_inviteOnly;
		bool				_topicRestricted;

	public:
		explicit Channel(const string& name);
		~Channel();

		void						addMember(Client* client);
		void						removeMember(Client* client);
		bool						isMember(Client* client) const;
		bool						isEmpty() const;
		size_t						size() const;

		void						addOperator(Client* client);
		void						removeOperator(Client* client);
		bool						isOperator(Client* client) const;

		void						invite(Client* client);
		bool						isInvited(Client* client) const;

		bool						hasTopic() const;
		void						setTopic(const string& topic, const string& setterNick);
		time_t						getTopicTime() const;
		const string				&getTopic() const;
		const string				&getTopicSetter() const;

		bool						isInviteOnly() const;
		void						setInviteOnly(bool on);
		bool						isTopicRestricted() const;
		void						setTopicRestricted(bool on);

		bool						hasKey() const;
		bool						checkKey(const string& key) const;
		void						setKey(const string& key);
		void						clearKey();

		bool						hasUserLimit() const;
		bool						isFull() const;
		void						setUserLimit(size_t limit);
		void						clearUserLimit();

		string						getModeString() const;

		const string				&getName() const;
		const map<int, Client*>		&getMembers() const;
};

#endif
