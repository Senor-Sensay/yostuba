#pragma once
#include <aegis.hpp>

template <typename T>
nlohmann::json get_config_value(const std::string& name, T default_value = T()) {
	std::ifstream config_stream(".\\config.json");
	nlohmann::json j;
	config_stream >> j;
	return j.value<T>(name, default_value);
}

#define DEFAULT_STATUS "with pointers | >help"
#define GPT_KEY "token"