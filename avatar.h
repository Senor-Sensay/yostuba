#pragma once
#include <aegis.hpp>
#include "commands/command.h"

extern aegis::core bot;

namespace Yotsuba {

	namespace Commands {
		class Ping : public Yotsuba::command {
		public:
			Ping() {
				this->aliases = { "ping", "pingalias" };
				this->usage = ">ping";
				this->description = "Displays the ping in milliseconds between Discord and the bot server.";
				this->required_perms = aegis::permissions::SEND_MESSAGES;
			}

			bool execute(Yotsuba::command_data* data, std::vector<std::string> args) {

				std::chrono::milliseconds msg_time(data->message->get_id().get_time());
				std::chrono::milliseconds current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock().now().time_since_epoch());

				std::chrono::milliseconds time_span = current_time - msg_time;

				std::stringstream ss;
				ss << "Pong! `" << std::abs(time_span.count()) << "ms`";

				aegis::future<aegis::gateway::objects::message> send_future = data->channel->create_message(ss.str());
				send_future.wait();

				return send_future.failed();
			}
		};
	}

}