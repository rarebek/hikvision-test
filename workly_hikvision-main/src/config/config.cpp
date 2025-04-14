#include "config.hpp"
#include <regex>

template <class T>
T read(const nlohmann::json &j, std::string key) {
    if (j.contains(key)) {
        return j.at(key);
    }
    throw std::runtime_error("\"" + key + "\" not found");
}


void config::parse_from(const std::string& filepath) {

	std::ifstream file(filepath);
	if (!file.is_open()) {
		throw std::runtime_error("couldn't open config file: " + filepath);
	}

	nlohmann::json config;
	file >> config;
	file.close();

	if (config.contains("pgdatabase")) {
		_db_name = config["pgdatabase"];
	}

	if (config.contains("pguser")) {
		_db_user = config["pguser"];
	}

	if (config.contains("pgpassword")) {
		_db_password = config["pgpassword"];
	}

	if (config.contains("pghostaddr")) {
		_db_address = config["pghostaddr"];
	}

	if (config.contains("pgport")) {
		_db_port = config["pgport"];
	}

	if (config.contains("pgsslmode")) {
		_db_ssl_mode = config["pgsslmode"];
	}

	if (config.contains("enable_debug_logs")) {
		_enable_debug_logs = config["enable_debug_logs"];
	}

	if (config.contains("enable_sdk_logs")) {
		_enable_sdk_logs = config["enable_sdk_logs"];
	}

	if (config.contains("keep_logs_days")) {
		_keep_logs_days = config["keep_logs_days"];
	}

	if (config.contains("check_logs_hours")) {
		_check_logs_hours = config["check_logs_hours"];
	}

	if (config.contains("token_file")) {
		_token_filename = config["token_file"];
	}

	_ehome_key = read<std::string>(config, "ehome_key");
	_public_ip = read<std::string>(config, "public_ip");
	_cms_server_port = read<uint16_t>(config, "cms_server_port");
	_ipc_server_port = read<uint16_t>(config, "ipc_server_port");
	_http_server_port = read<uint16_t>(config, "http_server_port");

	auto storage_servers = read<nlohmann::json>(config, "storage_servers");
	auto alarm_servers = read<nlohmann::json>(config, "alarm_servers");
	std::regex re("(\\d{1,3}.\\d{1,3}.\\d{1,3}.\\d{1,3}):(\\d{1,5})");
	
	for (std::string item : storage_servers) {
		std::smatch match;
		if (std::regex_match(item, match, re) && match.ready()) {
			_storage_servers.push_back(
				std::make_pair(match[1].str(), (uint16_t)std::atol(match[2].str().c_str()))
			);
		}
	}

	for (std::string item : alarm_servers) {
		std::smatch match;
		if (std::regex_match(item, match, re) && match.ready()) {
			_alarm_servers.push_back(
				std::make_pair(match[1].str(), (uint16_t)std::atol(match[2].str().c_str()))
			);
		}
	}

	_images_directory = read<std::string>(config, "images_directory");
	_records_directory = read<std::string>(config, "records_directory");
	_logs_directory = read<std::string>(config, "logs_directory");

	if (_records_directory.empty()) {
		throw std::runtime_error("records_directory is not set");
	}
	if (_images_directory.empty()) {
		throw std::runtime_error("images_directory is not set");
	}
	if (_logs_directory.empty()) {
		throw std::runtime_error("logs_directory is not set");
	}

	// end all directories with /
	if (_records_directory.back() != '/') {
		_records_directory += '/';
	}
	if (_images_directory.back() != '/') {
		_images_directory += '/';
	}
	if (_logs_directory.back() != '/') {
		_logs_directory += '/';
	}

	_temp_images_directory = _images_directory + "temp/";

	// check all directories
	if (!std::filesystem::create_directories(_records_directory) && !std::filesystem::is_directory(_records_directory)) {
		throw std::runtime_error("failed to create records_directory");
	}
	if (!std::filesystem::create_directories(_images_directory) && !std::filesystem::is_directory(_images_directory)) {
		throw std::runtime_error("failed to create images_directory");
	}
	if (!std::filesystem::create_directories(temp_images_directory()) && !std::filesystem::is_directory(temp_images_directory())) {
		throw std::runtime_error("failed to create temp_images_directory");
	}
	if (!std::filesystem::create_directories(_logs_directory) && !std::filesystem::is_directory(_logs_directory)) {
		throw std::runtime_error("failed to create logs_directory");
	}
}

std::string config::_logs_directory;
std::string config::_images_directory;
std::string config::_temp_images_directory;
std::string config::_records_directory;
std::string config::_public_ip;
std::string config::_ehome_key;
uint16_t config::_cms_server_port;
uint16_t config::_ipc_server_port;
uint16_t config::_http_server_port;
std::string config::_db_address = "localhost";
std::string config::_db_port = "5432";
std::string config::_db_name = "workly";
std::string config::_db_user = "dahualistener";
std::string config::_db_password = "K)56g5dr:HHj";
std::string config::_db_ssl_mode = "allow";
bool config::_enable_debug_logs = false;
bool config::_enable_sdk_logs = false;
int config::_keep_logs_days = -1;
int config::_check_logs_hours = 5;
std::string config::_token_filename;
std::vector<std::pair<std::string, uint16_t>> config::_storage_servers;
std::vector<std::pair<std::string, uint16_t>> config::_alarm_servers;