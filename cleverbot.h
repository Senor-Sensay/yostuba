#pragma once
#include <aegis.hpp>
#include "../message_utils.h"
#include <regex>
#include "md5.h"

std::unordered_map<aegis::snowflake, std::vector<std::string>> context_buffers;

std::string get_cleverbot_response(std::string message, Yotsuba::command_data data) {

	aegis::rest::rest_controller rc("", &bot.get_io_context());
	aegis::snowflake userID = data.user->get_id();
	std::string username = data.user->get_username();

	message = std::regex_replace(message, std::basic_regex("<@!?[0-9]+>\\s*"), "");

	bool hasContext = context_buffers.find(userID) != context_buffers.end();
	
	std::string context;
	if (hasContext)
		context = fmt::format("{}\n{}:{}", join(context_buffers[userID], "\n"), username, message);
	else
		context = fmt::format("{}:{}", username, message);

	bot.log->info(context);

	config_manager mgr = get_manager(data.guild->get_id());

	std::string personality = wstr_to_str(mgr.get_config<std::wstring>("personalities", "personality", L"friendly and helpful"));
	float temperature = mgr.get_config<float>("temperatures", "temperature", 0.5f);

	nlohmann::json payload = {
		{"prompt", fmt::format("The following is a conversation with an AI named Yotsuba. The AI is {}.\n{}\nAI:", personality, context)},
		{"temperature", temperature},
		{"max_tokens", 100},
		{"top_p", 1},
		{"frequency_penalty", 0.8f},
		{"presence_penalty", 0.8f},
		{"stop", {fmt::format("{}:", username), "AI:", "\n"}}
	};

	aegis::rest::request_params rp;
	rp.method = aegis::rest::RequestMethod::Post;
	rp.path = "/v1/engines/curie/completions";
	rp.host = "api.openai.com";
	rp.headers = { "Content-Type: application/json", fmt::format("Authorization: Bearer {}", GPT_KEY) };
	rp.body = payload.dump();
	websocketpp::http::parser::response rr = rc.custom_execute(std::forward<aegis::rest::request_params>(rp));
	nlohmann::json response_json = nlohmann::json::parse(rr.get_body());
	std::string response;

	if (!response_json["choices"].empty()) {
		response = static_cast<std::string>(response_json["choices"][0]["text"]);

		// Filter response
		response = std::regex_replace(response, std::basic_regex("@here|@everyone", std::regex_constants::icase), ""); // remove mass pings

		if (!hasContext) {
			context_buffers[userID] = { fmt::format("{}:{}\nAI:{}", username, message, response) };
		}
		else {
			context_buffers[userID].push_back({ fmt::format("{}:{}\nAI:{}", username, message, response) });
			if (context_buffers[userID].size() > 3)
				context_buffers[userID].erase(context_buffers[userID].begin());
		}
	}
	else response = "AI error.";

	return response;

		
}