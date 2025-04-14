#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <ctime>
#include <string>
#include <list>
#include <vector>
#include <cstdarg>
#include <fstream>
#include <map>
#include <filesystem>
#include <iostream>
#include <regex>
#include "../date/date.hpp"

namespace mylib {

	class logger {
		logger();
		~logger();

	public:
		static logger& instance();

		void set_directory(std::string path);
		void set_enable_debug_logs(bool enable);
		void set_cleaner_params(int keep_logs_days, int check_logs_hours);

		enum class level { error, info, debug };

		void start();
		void stop();
		void write(logger::level level, std::string module, const char* format, ...);

	private:
		void process();
		std::string get_logfile_path();
		std::string get_level_as_string(level l);

	private:
		struct log {
			logger::level level;
			std::time_t timestamp;
			std::string module;
			std::string message;
		};

		bool _started = false;
		std::thread _thr;
		std::mutex _mtx;
		std::condition_variable _cv;
		std::list<log> _logs;
		std::string _directory_path;
		bool _print_debug_logs = false;

		std::thread _thr_log_cleaner;
		std::condition_variable _cv_log_cleaner;
		std::mutex _mtx_log_cleaner;
		bool _is_cleaner_running = false;
		void cleaner();
		int _keep_logs_days = -1;
		int _check_logs_hours = 5;
	};

}

#define log_error(MODULE, ...) mylib::logger::instance().write(mylib::logger::level::error, MODULE, __VA_ARGS__)
#define log_info(MODULE, ...) mylib::logger::instance().write(mylib::logger::level::info, MODULE, __VA_ARGS__)
#define log_debug(MODULE, ...) mylib::logger::instance().write(mylib::logger::level::debug, MODULE, __VA_ARGS__)

