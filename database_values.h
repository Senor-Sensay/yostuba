#pragma once
#include <aegis.hpp>
#include "sql_utils.h"
#include "message_utils.h"
#include "commands/utils/backup_manager.h"
#include "commands/utils/config_manager.h"
#include "commands/utils/purge_util.h"
#include "commands/utils/protection_manager.h"

std::unordered_map<aegis::snowflake, uint64_t> running_timers {};

std::unordered_map<std::pair<aegis::snowflake, aegis::snowflake>, uint64_t, hash_pair> mute_timers {};

std::unordered_map<std::string, backup_manager<std::chrono::hours>*> backup_managers {};

std::vector<purge_manager*> auto_purges {};

std::unordered_map<aegis::snowflake, protection_manager*> spam_protections;

std::unordered_map<aegis::snowflake, protection_manager*> nuke_protections;

bool db_values_loaded = false;

void load_timers() {

	otl_stream* timer_stream = query_db("SELECT * FROM timers");

	aegis::snowflake userID;
	int deadline;
	std::wstring w_msg;

	while (!timer_stream->eof()) {

		std::wstring w_userID;
		*timer_stream >> w_userID;
		userID = aegis::snowflake(wstr_to_str(w_userID));
		*timer_stream >> deadline;
		*timer_stream >> w_msg;

		std::chrono::minutes diff = (std::chrono::minutes(deadline) - get_current_time<std::chrono::minutes>());

		if (diff.count() <= 0) { //If timer deadline already passed, send message immediately and let user know hot late it is
			int count = std::abs(diff.count());
			std::string unit;
			if (count >= 1440) {
				count /= 1440;
				unit = "day";
			}
			else if (count >= 60) {
				count /= 60;
				unit = "hour";
			}
			else {
				unit = "minute";
			}

			if (count != 0)
				bot.create_dm_message(userID,
					fmt::format("Sorry, but due to the bot being offline, this timer has expired approximately {} {}(s) late. \
The message for the timer was as follows:\n{}", count, unit, wstr_to_str(w_msg))).wait();
			else
				bot.create_dm_message(userID, wstr_to_str(w_msg));
			query_db_direct(fmt::format("delete from timers where id = {}", userID.gets()).c_str());
		}
		else { //If timer still ticking, wait to activate
			running_timers[userID] = bot.set_timeout([userID, w_msg]() mutable {
				bot.create_dm_message(userID, wstr_to_str(w_msg)).wait();
				query_db_direct(fmt::format("delete from timers where id = {}", userID.gets()).c_str());
			}, std::chrono::milliseconds(diff));
		}
	}
	delete timer_stream;
}

void load_backup_timers() {
	otl_stream* stream = query_db("SELECT * FROM backuptimers");
	while (!stream->eof()) {
		std::wstring hash;
		int deadline;
		int interval;
		*stream >> hash;
		*stream >> deadline;
		*stream >> interval;
		backup_managers[wstr_to_str(hash)] = new backup_manager(wstr_to_str(hash), (std::chrono::hours(deadline) - get_current_time<std::chrono::hours>()), std::chrono::hours(interval));
	}
	delete stream;
}

void load_purge_timers() {
	otl_stream* stream = query_db("SELECT * FROM purgetimers");
		while (!stream->eof()) {
			std::wstring w_channelID;
			aegis::channel* channel;
			aegis::snowflake channelID;
			int deadline;
			int interval;

			*stream >> w_channelID;
			channelID = aegis::snowflake(wstr_to_str(w_channelID));
			channel = bot.find_channel(channelID);
			*stream >> deadline;
			*stream >> interval;
			if (channel == nullptr) { // Channel doesn't exist, delete purge timer
				query_db_direct(fmt::format("DELETE FROM purgetimers WHERE id = \"{}\"", channelID.gets()).c_str());
				continue;
			}

			auto_purges.push_back(new purge_manager(channelID, deadline, interval, true));
			
		}
		delete stream;
}

void load_mute_timers() {

	otl_stream* timer_stream = query_db("SELECT * FROM mutes");

	aegis::snowflake serverID;
	aegis::snowflake userID;
	int deadline;
	std::wstring w_msg;

	if (timer_stream != nullptr) {

		while (!timer_stream->eof()) {

			std::wstring w_userID;
			std::wstring w_serverID;
			*timer_stream >> w_serverID;
			*timer_stream >> w_userID;
			serverID = aegis::snowflake(wstr_to_str(w_serverID));
			userID = aegis::snowflake(wstr_to_str(w_userID));
			*timer_stream >> deadline;
			*timer_stream >> w_msg;

			aegis::guild* guildPtr = bot.find_guild(serverID);

			if (guildPtr == nullptr) continue;

			aegis::snowflake mutedID(wstr_to_str(get_manager(serverID).get_config<std::wstring>("muteroles", "roleid", L"0")));

			if (mutedID == 0) {
				if (guildPtr->find_role("muted").has_value()) {
					mutedID = guildPtr->find_role("muted").value().id;
				}
				else
					continue;
			}

			std::chrono::minutes diff = (std::chrono::minutes(deadline) - get_current_time<std::chrono::minutes>());

			if (diff.count() <= 0) { //If unmute deadline already passed, unmute immediately and let user know hot late it is
				int count = std::abs(diff.count());
				std::string unit;
				if (count >= 1440) {
					count /= 1440;
					unit = "day";
				}
				else if (count >= 60) {
					count /= 60;
					unit = "hour";
				}
				else {
					unit = "minute";
				}

				if (count != 0) {
					if (bot.find_user(userID) != nullptr) {
						bot.create_dm_message(userID,
							fmt::format("Sorry, but due to the bot being offline, you are being unmuted approximately {} {}(s) late. \
The reason for your mute was as follows:\n{}", count, unit, wstr_to_str(w_msg))).wait();
						guildPtr->remove_guild_member_role(userID, mutedID);
					}
				}
				else
					guildPtr->remove_guild_member_role(userID, mutedID);

				query_db_direct(fmt::format("delete from mutes where id = \"{}\" and userid = \"{}\"", serverID.gets(), userID.gets()).c_str());
			}
			else { //If timer still ticking, wait to unmute
				mute_timers[{serverID, userID}] = bot.set_timeout([mutedID, guildPtr, serverID, userID]() {
					guildPtr->remove_guild_member_role(userID, mutedID);
					query_db_direct(fmt::format("delete from mutes where id = \"{}\" and userid=\"{}\"", serverID.gets(), userID.gets()).c_str());
				}, std::chrono::milliseconds(diff));
			}
		}
	}
	delete timer_stream;
}

void load_protections() {

	//otl_stream* stream = query_db("SELECT * FROM antinuke_managers");

	std::wstring w_serverID;
	int rate_limit;
	int violation_limit;
	int punishment;

	/*if (stream != nullptr) {
		while (!stream->eof()) {
			*stream >> w_serverID;
			*stream >> rate_limit;
			*stream >> violation_limit;
			*stream >> punishment;
			aegis::snowflake serverID = aegis::snowflake(wstr_to_str(w_serverID));
			nuke_protections[serverID] = new protection_manager(serverID, std::chrono::seconds(rate_limit), violation_limit, (violation_punishment_t)punishment);
		}
	}
	delete stream;*/

	std::wstring w_msg;
	int mute_time;
	std::wstring w_unit;

	otl_stream* stream = query_db("SELECT * FROM antispam_managers");
	if (stream != nullptr) {
		while (!stream->eof()) {
			*stream >> w_serverID;
			*stream >> rate_limit;
			*stream >> violation_limit;
			*stream >> punishment;
			*stream >> w_msg;
			*stream >> mute_time;
			*stream >> w_unit;
			aegis::snowflake serverID = aegis::snowflake(wstr_to_str(w_serverID));
			spam_protections[serverID] = new protection_manager(serverID, std::chrono::milliseconds(rate_limit), violation_limit, (violation_punishment_t)punishment);
			if (!w_msg.empty()) spam_protections[serverID]->mute_msg = wstr_to_str(w_msg);
			if (mute_time != 0) spam_protections[serverID]->mute_time = mute_time;
			if (!w_unit.empty()) spam_protections[serverID]->mute_unit = wstr_to_str(w_unit);
		}
	}
	delete stream;
}

void load_db_values() {

	bot.log->info("LOADING DB VALUES");

	load_timers();
	load_backup_timers();
	load_mute_timers();
	load_purge_timers();
	load_protections();

	db_values_loaded = true;
	bot.log->info("FINISHED LOADING DB VALUES");
}