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

		const string				&getName() const;
		const map<int, Client*>		&getMembers() const;
};

#endif
