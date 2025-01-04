#pragma once
#include <aegis.hpp>
#include <iostream>
#include "../database_values.h"

void to_lower(std::string& str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
}

void to_upper(std::string& str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::toupper(c); });
}

std::string to_lower_copy(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
	return str;
}

std::string to_upper_copy(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::toupper(c); });
	return str;
}

char* multi_tok(char* input, char* delimiter) {
	static char* string;
	if (input != NULL)
		string = input;

	if (string == NULL)
		return string;

	char* end = strstr(string, delimiter);
	if (end == NULL) {
		char* temp = string;
		string = NULL;
		return temp;
	}

	char* temp = string;

	*end = '\0';
	string = end + strlen(delimiter);
	return temp;
}

std::vector<std::string> split(const std::string& str, const char* delim) {
	char* context;
	char* current = multi_tok(const_cast<char*>(str.c_str()), const_cast<char*>(delim));
	std::vector<std::string> substrs;
	while (current != nullptr) {
		substrs.push_back(current);
		current = multi_tok(nullptr, const_cast<char*>(delim));
	}
	return substrs;
}

std::vector<std::string> split_w(const std::wstring& str, const wchar_t* delim) {
	wchar_t* context;
	wchar_t* current = wcstok_s(const_cast<wchar_t*>(str.c_str()), delim, &context);
	std::vector<std::string> substrs;
	while (current != nullptr) {
		substrs.push_back(wstr_to_str(current));
		current = wcstok_s(nullptr, delim, &context);
	}
	return substrs;
}

std::list<std::string> split_list(const std::string& str, const char* delim) {
	char* context;
	char* current = multi_tok(const_cast<char*>(str.c_str()), const_cast<char*>(delim));
	std::list<std::string> substrs;
	while (current != nullptr) {
		substrs.push_back(current);
		current = multi_tok(const_cast<char*>(str.c_str()), const_cast<char*>(delim));
	}
	return substrs;
}

std::list<std::string> split_list_w(const std::wstring& str, const wchar_t* delim) {
	wchar_t* context;
	wchar_t* current = wcstok_s(const_cast<wchar_t*>(str.c_str()), delim, &context);
	std::list<std::string> substrs;
	while (current != nullptr) {
		substrs.push_back(wstr_to_str(current));
		current = wcstok_s(nullptr, delim, &context);
	}
	return substrs;
}

std::string join(const std::vector<std::string> &str_vector, const std::string& join_chars, const unsigned& start_pos = 0) {
	if (str_vector.empty() || start_pos > str_vector.size()) return "";
	std::stringstream out;
	unsigned current_pos = start_pos;
	std::for_each(str_vector.begin() + start_pos, str_vector.end(), [&out, &join_chars, &start_pos, &current_pos](std::string s) {
		out << ((current_pos == start_pos) ? "" : join_chars) << s; 
		++current_pos;
	});
	return out.str();
}

double get_similarity(std::string string1, std::string string2, bool case_sensitive = false) {
	size_t size1 = string1.size();
	size_t size2 = string2.size();
	double matched = 0;
	double total = max(size1, size2);
	if (!case_sensitive) {
		to_lower(string1);
		to_lower(string2);
	}
	for (int i = 0; i < min(size1, size2); ++i)
		if (string1[i] == string2[i]) matched += 1;
	return (matched / total);
}