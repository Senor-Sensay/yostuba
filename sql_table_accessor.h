#pragma once
#include <aegis.hpp>
#include "sql_utils.h"
#include "string_utils.h"

class sql_table_accessor {

public:

	sql_table_accessor(const char* table_name) : table_name(table_name) {};

	template <typename value_type>
	void get_db_value(aegis::snowflake id, const char* value_name, value_type& out, value_type&& def = value_type()) {
		if (id == 0) {
			out = def;
			return;
		}
		std::wstring w_id = str_to_wstr(id.gets());
		otl_stream* stream = query_db(fmt::format("SELECT {} FROM {} WHERE id = :id<char[19]>", value_name, table_name).c_str(), 1, w_id);
		if (stream == nullptr || stream->eof()) {
			delete stream;
			out = def;
			return;
		}
		else {
			*stream >> out;
			delete stream;
		}
	}

	template <typename... value_types>
	bool row_exists(aegis::snowflake id, const char* query_text = "", value_types&&... vals) {
		std::wstring w_id = str_to_wstr(id.gets());
		otl_stream* stream;
		if (query_text != "")
			stream = query_db(fmt::format("SELECT EXISTS(SELECT * FROM {} WHERE id = :id<char[19]> AND {})", this->table_name, query_text).c_str(), 1, w_id, vals...);
		else
			stream = query_db(fmt::format("SELECT EXISTS(SELECT * FROM {} WHERE id = :id<char[19]>)", this->table_name).c_str(), 1, w_id);

		if (stream == nullptr) return false;
		int row_exists;
		*stream >> row_exists;
		delete stream;
		return row_exists;
	}

	template <typename... value_types>
	unsigned row_count(aegis::snowflake id, const char* query_text = "", value_types&&... vals) {
		std::wstring w_id = str_to_wstr(id.gets());
		otl_stream* find_stream;
		if (query_text != "")
			find_stream = query_db(fmt::format("SELECT COUNT(*) FROM {} WHERE id = :id<char[19]> AND {}", this->table_name, query_text).c_str(), 1, w_id, vals...);
		else
			find_stream = query_db(fmt::format("SELECT COUNT(*) FROM {} WHERE id = :id<char[19]>", this->table_name).c_str(), 1, w_id);

		unsigned count = 0;
		if (find_stream != nullptr) {
			*find_stream >> count;
			delete find_stream;
		}
		return count;
	}

	template <typename... value_types>
	bool insert_db_value(aegis::snowflake id, const char* query_text, value_types&&... vals) {

		std::wstring w_id = str_to_wstr(id.gets());

		otl_stream* stream = query_db(fmt::format("INSERT INTO {} VALUES (:f1<char[19]>, {})", this->table_name, query_text).c_str(), 1, w_id, vals...);

		if (stream != nullptr) {
			delete stream;
			return true;
		}
		else return false;
	}

	template <typename... value_types>
	bool insert_db_value_specific(aegis::snowflake id, std::vector<const char*> column_names, const char* query_text, value_types&&... vals) {

		std::wstring w_id = str_to_wstr(id.gets());

		otl_stream* stream = query_db(fmt::format("INSERT INTO {} (id, {}) VALUES (:f1<char[19]>, {})", this->table_name, ::join(column_names, ", "), query_text).c_str(), 1, w_id, vals...);

		if (stream != nullptr) {
			delete stream;
			return true;
		}
		else return false;
	}

	template <typename... value_types>
	bool update_db_value(aegis::snowflake id, const char* query_text, value_types&&... vals) {

		std::wstring w_id = str_to_wstr(id.gets());

		otl_stream* stream = query_db(fmt::format("UPDATE {} SET {} WHERE id = :id<char[19]>", this->table_name, query_text).c_str(), 1, vals..., w_id);

		if (stream != nullptr) {
			delete stream;
			return true;
		}
		else return false;
	}

	bool delete_db_value(aegis::snowflake id, const char* query_text = "") {
		if (query_text != "")
			return query_db_direct(fmt::format("DELETE FROM {} WHERE ID = \"{}\" AND {}", table_name, id.gets(), query_text).c_str());
		else
			return query_db_direct(fmt::format("DELETE FROM {} WHERE ID = \"{}\"", table_name, id.gets()).c_str());
	}

private:

	const char* table_name;

};