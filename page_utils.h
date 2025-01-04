#pragma once
#include <aegis.hpp>
#include "string_utils.h"

class page_display {
public:
	page_display(const std::string& text, const char* delim, const unsigned groups_per_page = 10) {
		std::vector<std::string> groups = split(text, delim);
		this->create_pages(groups, groups_per_page);
	}

	page_display(const std::vector<std::string>& groups, const unsigned groups_per_page = 10) {
		this->create_pages(groups, groups_per_page);
	}

	std::vector<std::string> pages;

private:
	void create_pages(const std::vector<std::string>& groups, const unsigned groups_per_page) {
		std::stringstream buffer;
		unsigned counter = 0;
		for (const std::string& group : groups) {
			buffer << group;
			counter++;
			if ((counter != 0 && counter % groups_per_page == 0) || counter == groups.size()) {
				this->pages.push_back(buffer.str());
				buffer.str("");
			}
		}
	}
};