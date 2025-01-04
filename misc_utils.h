#include <aegis.hpp>
#include "message_utils.h"

aegis::channel* purge(aegis::channel* target) {

	aegis::guild* guild = &target->get_guild();
	aegis::snowflake channelID = target->get_id();
	aegis::snowflake serverID = guild->get_id();

	if (guild == nullptr || target == nullptr) return nullptr;

	aegis::modify_channel_t modifier = aegis::modify_channel_t()\
		.bitrate(target->get_bitrate())\
		.name(target->get_name())\
		.nsfw(target->nsfw())\
		.parent_id(target->get_parent_id())\
		.permission_overwrites(values(target->get_overwrites_nocopy()))\
		.position(target->get_position())\
		.rate_limit_per_user(target->get_rate_limit_per_user())\
		.topic(target->get_topic())\
		.user_limit(target->get_user_limit());

	aegis::channel_type type = target->get_type();

	target->delete_channel();

	aegis::gateway::objects::channel new_channel;

	switch (type) {
	case aegis::channel_type::Category:
		new_channel = guild->create_category_channel(modifier._name.value(), modifier._parent_id.value(), modifier._permission_overwrites.value()).get();
		break;
	case aegis::channel_type::Text:
		new_channel = guild->create_text_channel(modifier._name.value(), modifier._parent_id.value(), modifier._nsfw.value(), modifier._permission_overwrites.value()).get();
		break;
	case aegis::channel_type::Voice:
		new_channel = guild->create_voice_channel(modifier._name.value(), modifier._bitrate.value(), modifier._user_limit.value(), modifier._parent_id.value(), modifier._permission_overwrites.value()).get();
		break;
	default:
		return nullptr;
		break;
	}

	while (bot.find_channel(new_channel.id) == nullptr) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	if (greeting_accessor.row_exists(serverID)) {
		std::wstring joinid;
		std::wstring mainid;
		greeting_accessor.get_db_value(serverID, "channel", joinid);
		mainid = get_manager(serverID).get_config<std::wstring>("mainchannels", "channelid", L"0");
		if (aegis::snowflake(wstr_to_str(joinid)) == channelID) { // If deleted channel was greet channel, auto-update greet table
			std::wstring joinmsg;
			greeting_accessor.get_db_value(serverID, "message", joinmsg);
			greeting_accessor.update_db_value(serverID, "channel = :channel<char[19]>, message = :message<char[201]>", str_to_wstr(new_channel.id.gets()), joinmsg);
		}
		if (wstr_to_str(mainid) == channelID.gets()) {
			get_manager(serverID).set_config("mainchannels", "channelid", ":channelid<char[19]>", str_to_wstr(new_channel.id.gets()));
		}
	}
	if (goodbye_accessor.row_exists(serverID)) {
		std::wstring byeid;
		greeting_accessor.get_db_value(serverID, "channel", byeid);
		if (aegis::snowflake(wstr_to_str(byeid)) == channelID) { // If deleted channel was bye channel, auto-update bye table
			std::wstring byemsg;
			goodbye_accessor.get_db_value(serverID, "message", byemsg);
			goodbye_accessor.update_db_value(serverID, "channel = :channel<char[19]>, message = :message<char[201]>", str_to_wstr(new_channel.id.gets()), byemsg);
		}
	}

	wait_for(bot.find_channel(new_channel.id)->modify_channel({}, modifier._position, modifier._topic, {}, {}, {}, {}, {}, modifier._rate_limit_per_user));

	return bot.find_channel(new_channel.id);
}