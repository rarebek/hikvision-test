#include "logger.hpp"

namespace mylib {

	logger& logger::instance() {
		static logger l;
		return l;
	}

	logger::logger() {}

	logger::~logger() {
		stop();
	}

	void logger::start() {

		if (_started) {
			return;
		}

		_started = true;
		_thr = std::thread(&logger::process, this);

		if (_keep_logs_days > 0) {
			_is_cleaner_running = true;
			_thr_log_cleaner = std::thread(&logger::cleaner, this);
		}
	}

	void logger::stop() {
		if (_started) {
			std::unique_lock<std::mutex> lock(_mtx);
			_started = false;
			_cv.notify_all();
			if (_thr.joinable()) {
				lock.unlock();
				_thr.join();
			}
		}

		if (_is_cleaner_running) {
			std::unique_lock<std::mutex> lock(_mtx_log_cleaner);
			_is_cleaner_running = false;
			_cv_log_cleaner.notify_one();
			if (_thr_log_cleaner.joinable()) {
				lock.unlock();
				_thr_log_cleaner.join();
			}
		}
	}

	void logger::set_directory(std::string path) {
		if (!path.empty()) {
			std::lock_guard<std::mutex> lock(_mtx);
			_directory_path = path;
			std::filesystem::create_directories(path);
		}
	}

	void logger::set_enable_debug_logs(bool enable) {
		_print_debug_logs = enable;
	}

	void logger::set_cleaner_params(int keep_logs_days, int check_logs_hours) {
		_keep_logs_days = keep_logs_days;
		_check_logs_hours = check_logs_hours;
	}

	void logger::write(logger::level level, std::string module, const char *format, ...) {

		if (level == level::debug && !_print_debug_logs) {
			return;
		}
		
		std::time_t now = std::time(nullptr);

		std::vector<char> buffer(10240);
		va_list args;
		va_start(args, format);
		vsprintf(buffer.data(), format, args);
		va_end(args);

		logger::log log;
		log.level = level;
		log.timestamp = now;
		log.module = module;
		log.message = buffer.data();

		std::lock_guard<std::mutex> lock(_mtx);
		_logs.push_back(log);
		_cv.notify_one();
	}

	std::string logger::get_logfile_path() {
		
		date now;
		
		char year[5], month[3], day[3];
		sprintf(year, "%04d", now.year());
		sprintf(month, "%02d", now.month());
		sprintf(day, "%02d", now.day());
		return _directory_path + year + month + day + ".log";
	}

	std::string logger::get_level_as_string(level l) {
		switch (l) {
		case level::error: return "[ERR]";
		case level::info:  return "[INF]";
		case level::debug: return "[DBG]";
		}
		return "";
	}

	void logger::process() {

		while (true) {

			// do not stop process, until all logs have been written
			std::unique_lock<std::mutex> lock(_mtx);
			if (_logs.empty()) {

				if (!_started) {
					break;
				}

				_cv.wait(lock);
				continue;
			}

			auto logs = std::move(_logs);
			lock.unlock();

			std::ofstream file(get_logfile_path(), std::ios_base::ate | std::ios_base::app);
			for (auto& log : logs) {

				// prepare log params
				std::string level = get_level_as_string(log.level);
				std::string time = date(log.timestamp).set_timezone(300).iso_string(true, true, true);

				// write log to file and console				
				if (log.module.empty()) {
					std::cout << "[" << time << "] " << level << log.message << std::endl;

					if (log.level == logger::level::error) {
						std::cerr << "[" << time << "]" << level << log.message << std::endl;
					}

					if (file.is_open()) {
						file << "[" << time << "] " << level << log.message << std::endl;
					}
				}
				else {
					std::cout << "[" << time << "]" << level << "[" << log.module << "] " << log.message << std::endl;

					if (log.level == logger::level::error) {
						std::cerr << "[" << time << "]" << level << "[" << log.module << "] " << log.message << std::endl;
					}

					if (file.is_open()) {
						file << "[" << time << "]" << level << "[" << log.module << "] " << log.message << std::endl;
					}
				}
			}

			if (file.is_open()) {
				file.close();
			}
		}

	}

	void logger::cleaner() {

		std::regex re("(\\d{8})\\.log$");

		while (true) {

			std::unique_lock<std::mutex> lock(_mtx_log_cleaner);
			if (!_is_cleaner_running) {
				break;
			}

			_cv_log_cleaner.wait_for(lock, std::chrono::hours(_check_logs_hours));
			long today = std::atoll(date().format("Ymd").c_str());
			for (auto& item : std::filesystem::directory_iterator(_directory_path)) {
				if (item.is_regular_file() && item.path().has_filename()) {
					std::string filename = item.path().filename().string();
					std::smatch match;
					if (std::regex_match(filename, match, re) && match.ready()) {
						long log_date = std::atoll(match[1].str().c_str());
						long diff = today - log_date;
						if (diff > _keep_logs_days) {
							log_debug("CLEANER", "removing log %s", item.path().c_str());
							std::remove(item.path().c_str());
						}
					}
				}	
			}
		}

	}

}