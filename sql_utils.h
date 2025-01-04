#pragma once
#include <aegis.hpp>
#include <stdio.h>
#define OTL_ODBC_MYSQL
// The following #define is required with MyODBC 5.1 and higher
//#define OTL_ODBC_SELECT_STM_EXECUTE_BEFORE_DESCRIBE
#define OTL_UNICODE // Compile OTL with Unicode 
#define OTL_UNICODE_CHAR_TYPE wchar_t
#define OTL_UNICODE_STRING_TYPE std::wstring

#include "otlv4.h"
#include <locale>

extern aegis::core bot;

otl_connect db;
static bool _db_initialized = false;

static std::mutex sql_mutex;

static std::queue<std::string> query_queue;

bool query_db_direct(std::string query);

void init_db() {

	if (_db_initialized) return;

	otl_connect::otl_initialize(0);

	// DO NOT TOUCH THIS UNLESS YOU WANT TO DIE, FAG
	try {
		db.rlogon("Driver = { MySQL ODBC 5.2 ANSI Driver }; Server = localhost; Database = yotsuba; User = root; Password = password; Option = 3;");
		_db_initialized = true;
	}
	catch (otl_exception ex) {
		bot.log->error(std::string(ex.msg, ex.msg + sizeof(ex.msg)));
	}

	if (db.connected) bot.log->info("Successfully connected to SQL server.");
	else bot.log->critical("Failed to connect to SQL server!");

	std::thread([] { // Query queue processing thread
		while (true) {
			if (!query_queue.empty()) {
				query_db_direct(query_queue.front());
				query_queue.pop();
			}
			std::this_thread::sleep_for(5s);
		}
	}).detach();

}

bool query_db_direct(std::string query) {

	sql_mutex.lock();

	if (!_db_initialized) {
		sql_mutex.unlock();
		return false;
	}

	try {
		otl_cursor::direct_exec(db, query.c_str());
		sql_mutex.unlock();
		return true;
	}
	catch (otl_exception ex) {
		bot.log->trace(std::string(ex.msg, ex.msg + sizeof(ex.msg)));
		sql_mutex.unlock();
		return false;
	}

}

bool queue_db_query(std::string query) {
	query_queue.push(query);
}

template <typename... in_types>
otl_stream* query_db(std::string query, unsigned out_buffer_size = 50, in_types&&... args) {

	sql_mutex.lock();

	if (!_db_initialized) { 
		sql_mutex.unlock(); 
		return nullptr; 
	}

	otl_stream* i = nullptr;

	try {
		i = new otl_stream(out_buffer_size, query.c_str(), db);

		((*i) << ... << std::forward<in_types>(args)); //Expand parameter pack as input into otl_stream
	}
	catch (otl_exception ex) {
		bot.log->trace(std::string(ex.msg, ex.msg + sizeof(ex.msg)));
	}

	sql_mutex.unlock();
	return i;
}

template <typename... value_types>
bool row_exists(std::string filter, std::string table_name, unsigned buf_size = 1, value_types&&... vals) {
	otl_stream* find_stream = query_db(fmt::format("SELECT EXISTS(SELECT * FROM {} WHERE {})", table_name, filter).c_str(), buf_size, vals...);
	int exists = 0;
	if (find_stream != nullptr) {
		*find_stream >> exists;
		delete find_stream;
	}
	return exists;
}

template <typename... value_types>
unsigned row_count(std::string filter, std::string table_name, unsigned buf_size = 1, value_types&&... vals) {
	otl_stream* find_stream = query_db(fmt::format("SELECT COUNT(*) FROM {} WHERE {}", table_name, filter).c_str(), buf_size, vals...);
	unsigned count = 0;
	if (find_stream != nullptr) {
		*find_stream >> count;
		delete find_stream;
	}
	return count;
}

static std::wstring_convert<std::codecvt<wchar_t, char, mbstate_t>> converter("Error.", L"Error.");

std::wstring str_to_wstr(std::string&& str) {
	return converter.from_bytes(std::forward<std::string&&>(str));
}

std::string wstr_to_str(std::wstring&& wstr) {
	return converter.to_bytes(std::forward<std::wstring&&>(wstr));
}

std::wstring str_to_wstr(std::string& str) {
	return converter.from_bytes(std::forward<std::string&&>(str));
}

std::string wstr_to_str(std::wstring& wstr) {
	return converter.to_bytes(std::forward<std::wstring&&>(wstr));
}