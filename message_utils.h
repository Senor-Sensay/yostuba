#pragma once
#include <aegis.hpp>
#include <random>

struct hash_pair {
	template <class T1, class T2>
	size_t operator()(const std::pair<T1, T2>& p) const
	{
		auto hash1 = std::hash<T1>{}(p.first);
		auto hash2 = std::hash<T2>{}(p.second);
		return hash1 ^ hash2;
	}
};

long long generate_random_int(const long long& min, const long long& max) {
	std::random_device rnd;
	std::uniform_int_distribution<long long> uid(min, max);
	return uid(rnd);
}

double generate_random_double(const double& min, const double& max) {
	std::random_device rnd;
	std::uniform_int_distribution<long long> uid(min, max);
	return uid(rnd);
}

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
std::string add_commas(T num) {
	int maxpos = -1;
	if (num < 0) //Prevent comma being added to sign in negative numbers
		maxpos = 0;
	std::string number_string = std::to_string(num);
	size_t count = 1;
	size_t original_length = number_string.length();
	for (size_t pos = original_length - 1; pos != maxpos; --pos, ++count) {
		if (count % 3 == 0 && count != original_length) {
			number_string.insert(pos, ",");
		}
	}
	return number_string;
}

	template <typename T>
	T find(const std::vector<T>& to_search, const std::function<bool(T)>& filter) {
		T out;
		
		for (const T in : to_search) {
			if (filter(in)) {
				out = in;
				break;
			}
		}

		return out;
	}

	template <typename T>
	T find(const std::list<T>& to_search, const std::function<bool(T)>& filter) {
		T out;

		for (const T in : to_search) {
			if (filter(in)) {
				out = in;
				break;
			}
		}

		return out;
	}

	template <typename T, typename... A>
	std::vector<T> filter(const std::vector<T>& to_search, const std::function<bool(T, A...)>& filter, A&... args) {
		std::vector<T> out;
		
		for (const T in : to_search) {
			if (filter(in, std::ref(args)...)) out.push_back(in);
		}

		return out;
	}

	template <typename T>
	std::vector<T> filter(const std::vector<T>& to_search, const std::function<bool(T)>& filter) {
		std::vector<T> out;

		for (const T in : to_search) {
			if (filter(in)) out.push_back(in);
		}

		return out;
	}

	template <typename T>
	std::list<T> filter(const std::list<T>& to_search, const std::function<bool(T)>& filter) {
		std::list<T> out;

		for (const T in : to_search) {
			if (filter(in)) out.push_back(in);
		}

		return out;
	}

	template <typename starting_type, typename mapped_type>
	std::vector<mapped_type> map(const std::vector<starting_type>& source, const std::function<mapped_type(starting_type)>& mapping_func) {
		std::vector<mapped_type> out;

		for (starting_type in : source) {
			out.push_back(mapping_func(in));
		}

		return out;
	}

	template <typename key_type, typename value_type, typename... types>
	std::pair<const key_type, value_type> find(const std::unordered_map<key_type, value_type, types...>& to_search, const std::function<bool(std::pair<key_type, value_type>)>& filter) {		
		for (auto& pair : to_search)
			if (filter(pair))
				return pair;
	}

	template <typename key_type, typename value_type>
	std::vector<std::pair<key_type, value_type>> filter(std::unordered_map<key_type, value_type> to_search, std::function<bool(std::pair<key_type, value_type>)> filter) {
		std::vector<std::pair<key_type, value_type>> out;
		
		for (const std::pair<key_type, value_type>& pair : to_search) {
			if (filter(pair)) out.push_back(pair);
		}

		return out;
	}

	template <typename key_type, typename value_type>
	std::vector<key_type> keys(std::unordered_map<key_type, value_type> to_search) {
		std::vector<key_type> out;
		for (const std::pair<const key_type, value_type>& pair : to_search) {
			out.push_back(pair.first);
		}
		return out;
	}

	template <typename key_type, typename value_type>
	std::vector<value_type> values(std::unordered_map<key_type, value_type> to_search) {
		std::vector<value_type> out;
		for (const std::pair<const key_type, value_type>& pair : to_search) {
			out.push_back(pair.second);
		}
		return out;
	}

	template <typename duration_type>
	duration_type get_current_time() {
		return std::chrono::duration_cast<duration_type>(std::chrono::system_clock().now().time_since_epoch());
	}

	template <typename func_type, typename rep, typename period, typename... arg_types>
	void set_timeout_cancelable(func_type&& func, std::atomic_bool& cancel_token, std::chrono::duration<rep, period> duration, arg_types&&... args) {
		
		auto callback = [func = std::forward<func_type>(func), args = std::make_tuple(std::forward<arg_types>(args)...)]() mutable { std::apply(func, std::move(args)); };
		std::chrono::duration end_time = get_current_time<std::chrono::duration<rep, period>>() + duration;

		std::thread t([=, &cancel_token]() mutable {
			while (cancel_token.load()) {
				std::this_thread::sleep_for(1s);
				try {
					if (cancel_token.load() && get_current_time<std::chrono::duration<rep, period>>() >= end_time) {
						callback();
						return;
					}
				}
				catch (std::exception) {}
			}
		});

		t.detach();

	}

	template <typename func_type, typename rep, typename period, typename... arg_types>
	void set_interval_cancelable(func_type&& func, std::atomic_bool& cancel_token, std::chrono::duration<rep, period> duration, bool do_first, arg_types&&... args) {

		auto callback = [func = std::forward<func_type>(func), args = std::make_tuple(std::forward<arg_types>(args)...)]() mutable { std::apply(func, std::move(args)); };
		std::chrono::duration end_time = get_current_time<std::chrono::duration<rep, period>>() + duration;

		if (do_first) callback();

		std::thread t([=, &cancel_token]() mutable {
			while (cancel_token.load()) {
				std::this_thread::sleep_for(1s);
				try {
					if (cancel_token.load() && get_current_time<std::chrono::duration<rep, period>>() >= end_time) {
						callback();
						end_time = get_current_time<std::chrono::duration<rep, period>>() + duration;
					}
				}
				catch (std::exception) {}
			}
		});

		t.detach();
	}

	template <typename func_type, typename rep, typename period, typename... arg_types>
	void set_timeout(func_type&& func, std::chrono::duration<rep, period> duration, bool do_first, arg_types&&... args) {

		auto callback = [func = std::forward<func_type>(func), args = std::make_tuple(std::forward<arg_types>(args)...)]() mutable { std::apply(func, std::move(args)); };

		if (do_first) callback();

		std::thread t([=]() mutable {
			std::this_thread::sleep_for(duration);
			try {
				callback();
			}
			catch (std::exception) {}
		});

		t.detach();
	}

	template <typename func_type, typename rep, typename period, typename... arg_types>
	void set_interval(func_type&& func, std::chrono::duration<rep, period> duration, bool do_first, arg_types&&... args) {

		auto callback = [func = std::forward<func_type>(func), args = std::make_tuple(std::forward<arg_types>(args)...)]() mutable { std::apply(func, std::move(args)); };

		if (do_first) callback();

		std::thread t([=]() mutable {
			while (true) {
				std::this_thread::sleep_for(duration);
				try {
					callback();
				}
				catch (std::exception) {}
			}
		});

		t.detach();
	}

