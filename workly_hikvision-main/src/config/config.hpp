#pragma once

#include <string>

#include "nlohmann/json.hpp"
#include <system_error>
#include <fstream>
#include <filesystem>
#include <vector>

class config {
	public:

	static void parse_from(const std::string& filepath);

	static const std::string& images_directory() { return _images_directory; }
	static const std::string& records_directory() { return _records_directory; }
	static const std::string& logs_directory() { return _logs_directory; }
	static const std::string temp_images_directory() { return _temp_images_directory; }

	static const std::string& public_ip() { return _public_ip; }
	static const std::string& ehome_key() { return _ehome_key; }
	
	static uint16_t cms_server_port() { return _cms_server_port; }
	static uint16_t ipc_server_port() { return _ipc_server_port; }
	static uint16_t http_server_port() { return _http_server_port; }

	static std::vector<std::pair<std::string, uint16_t>>& storage_servers() { return _storage_servers; }
	static std::vector<std::pair<std::string, uint16_t>>& alarm_servers() { return _alarm_servers; }

	static const std::string& db_address() { return _db_address; }
	static const std::string& db_port() { return _db_port; }
	static const std::string& db_name() { return _db_name; }
	static const std::string& db_user() { return _db_user; }
	static const std::string& db_password() { return _db_password; }
	static const std::string& db_ssl_mode() { return _db_ssl_mode; }

	static bool enable_debug_logs() { return _enable_debug_logs; }
	static bool enable_sdk_logs() { return _enable_sdk_logs; }
	static int keep_logs_days() { return _keep_logs_days; }
	static int check_logs_hours() { return _check_logs_hours; }

	static const std::string& token_filename() { return _token_filename; }

private:
	static std::string _logs_directory;
	static std::string _images_directory;
	static std::string _temp_images_directory;
	static std::string _records_directory;

	static std::string _public_ip;
	static std::string _ehome_key;
	static uint16_t _cms_server_port;
	static uint16_t _ipc_server_port;
	static uint16_t _http_server_port;

	static std::vector<std::pair<std::string, uint16_t>> _storage_servers;
	static std::vector<std::pair<std::string, uint16_t>> _alarm_servers;

	static std::string _db_address;
	static std::string _db_port;
	static std::string _db_name;
	static std::string _db_user;
	static std::string _db_password;
	static std::string _db_ssl_mode;

	static bool _enable_debug_logs;
	static bool _enable_sdk_logs;
	static int _keep_logs_days;
	static int _check_logs_hours;
	static std::string _token_filename;
};