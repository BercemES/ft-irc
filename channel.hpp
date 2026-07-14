#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <string>
# include <map>
# include <cstddef>

class Client;

using std::string;
using std::map;

class Channel
{
	private:
		string				_name;
		map<int, Client*>	_members;

	public:
		explicit Channel(const string& name);
		~Channel();

		void						addMember(Client* client);
		void						removeMember(Client* client);
		bool						isMember(Client* client) const;
		bool						isEmpty() const;
		size_t						size() const;

		const string				&getName() const;
		const map<int, Client*>		&getMembers() const;
};

#endif
