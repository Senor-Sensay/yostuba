#include "database_values.h"
#include "commands/command.h"
#include "cleverbot.h"
#include <regex>

//#pragma execution_character_set("utf-8")

static const std::vector<std::string> emojis = split(u8"😀 😁 😂 🤣 😃 😄 😅 😆 😉 😊 😋 😎 😍 😘 🥰 😗 😙 😚 ☺️ 🙂 🤗 🤩 🤔 🤨 😐 😑 😶 🙄 😏 \
😣 😥 😮 🤐 😯 😪 😫 😴 😌 😛 😜 😝 🤤 😒 😓 😔 😕 🙃 🤑 😲 ☹️ 🙁 😖 😞 😟 😤 😢 😭 😦 \
😧 😨 😩 🤯 😬 😰 😱 🥵 🥶 😳 🤪 😵 😡 😠 🤬 😷 🤒 🤕 🤢 🤮 🤧 😇 🤠 🤡 🥳 🥴 🥺 🤥 🤫 🤭 🧐 \
🤓 😈 👿 👹 👺 💀 👻 👽 🤖 💩 😺 😸 😹 😻 😼 😽 🙀 😿 😾", " ");

static sql_table_accessor nwt("nwordtable");

static const aegis::snowflake shitpost_id("843224142589067294");

std::unordered_map<std::pair<aegis::snowflake, aegis::snowflake>, std::function<void(Yotsuba::command_data&)>, hash_pair> response_catchers;

inline void filter_invites(Yotsuba::command_data& data) {
	if (!data.guild->get_permissions(data.user, data.channel).is_admin() && data.user->get_id() != bot.get_id()) { // Don't filter links from admins or ourself
		if (to_lower_copy(data.message->get_content()).find("discord.gg") != std::string::npos) {
				data.channel->delete_message(data.message->get_id());
				data.message->set_content(std::regex_replace(data.message->get_content(), std::basic_regex("discord\\.gg\\S*", std::regex_constants::icase), ""));
				data.channel->create_message(fmt::format("{}, invite links are not allowed.", data.user->get_mention()));
		}
	}
}

inline void filter_mass_pings(Yotsuba::command_data& data) {
	if (!data.guild->get_permissions(data.user, data.channel).is_admin())
		data.message->set_content(std::regex_replace(data.message->get_content(), std::basic_regex("@here|@everyone", std::regex_constants::icase), ""));
}

inline void find_n_words(Yotsuba::command_data& data) {
	std::string msg_content = to_lower_copy(data.message->get_content());
	aegis::snowflake userID = data.user->get_id();
	size_t pos = msg_content.find("nigger");
	if (pos != std::string::npos) {
		int count = 0;
		while (pos != std::string::npos) {
			++count;
			pos = msg_content.find("nigger", pos + 1);
		}
		if (nwt.row_exists(userID)) {
			queue_db_query(fmt::format("update nwordtable set count = count + {} where id = {}", count, userID).c_str());
		}
		else {
			queue_db_query(fmt::format("insert into nwordtable values ({},{})", userID, count).c_str());
		}
	}
}

inline std::thread* check_cleverbot(Yotsuba::command_data& data) {

	if (data.user->get_id() == bot.get_id()) return nullptr; // Do nothing if message is from us.

	bool shitpostRNG = false;
	bool emojiRNG = (generate_random_int(0, 100) == 50);
	if (emojiRNG) { // Random reacts
		std::vector<std::string> all_emotes = emojis;
		for (auto& emoji : data.guild->get_emojis_nocopy())
			all_emotes.push_back(emoji.second.reaction());
		data.message->create_reaction(all_emotes[generate_random_int(0, all_emotes.size() - 1)]);
	}

	if (data.message->get_content().empty()) return nullptr; // Only do AI responses if there is a message to respond to.

	if (data.guild->get_id() == shitpost_id) {
		shitpostRNG = (generate_random_int(0, 100) == 50); // roughly 1% chance of random cleverbot response
	}
	if (shitpostRNG || (data.message->mentions.size() != 0 && data.message->mentions[0] == bot.get_id())) { // Trigger cleverbot on bot ping
		return new std::thread([data]() { //Perform cleverbot response on seperate thread
			std::string response = get_cleverbot_response(data.message->get_content(), data);
			data.channel->create_message(fmt::format("{} {}", data.user->get_mention(), response));
		});
	}
	else return nullptr;
}

inline void catch_message(Yotsuba::command_data& data) {
	if (data.user->get_id() == bot.get_id()) return; // Do not catch messages from self
	auto catcher_func = response_catchers.find(std::pair<aegis::snowflake, aegis::snowflake>(data.channel->get_id(), data.user->get_id()));
	if (catcher_func != response_catchers.end()) {
		auto response_func = catcher_func->second;
		response_catchers.erase(catcher_func); // Remove catcher func after call
		response_func(data); // Call catcher func w/ data
	}
	else return; // No catcher func found
}

void process_message(Yotsuba::command_data& msg_data) {
	filter_mass_pings(msg_data);
	filter_invites(msg_data);
	catch_message(msg_data);
	find_n_words(msg_data);
}