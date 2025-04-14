#include "device_manager.hpp"
#include "../logger/logger.hpp"
#include "../config/config.hpp"
#include "../encoders/base64/base64.hpp"
#include "../db/db.hpp"
#include <list>
#include <filesystem>
#include <fstream>
#include <regex>
#include <algorithm>

device_manager::device_manager(long conn_id, std::string device_sn, std::string device_id, std::string device_ip, unsigned short device_port) {
	_device = std::make_shared<device>(conn_id, device_sn, device_id, device_ip, device_port);
	_running = false;
	_connected = false;
}

device_manager::~device_manager() {

	log_info(_device->serial_number(), "[device_manager][%lld] cleaning up", _device->conn_id());
	
	{
		std::lock_guard<std::mutex> lock(_mtx);
		_running = false;
		_cv.notify_all();
	}

	if (_connected) {
		_connected = false;
		_device->logout();
	}

	if (_thr.joinable()) {
		_thr.join();
	}

	log_info(_device->serial_number(), "[device_manager][%lld] cleaning up done", _device->conn_id());
}

bool device_manager::run() {

	log_info(_device->serial_number(), "[device_manager][%lld] running process", _device->conn_id());
	std::lock_guard<std::mutex> lock(_mtx);

	if (_running) {
		return true;
	}

	try {
		log_info(_device->serial_number(), "[device_manager][%lld] getting device info", _device->conn_id());
		db::instance().get_device_info(_device->serial_number(), _info);
		db::instance().update_device_name(_device->serial_number(), _device->name());
		db::instance().update_last_request_time(
			_device->serial_number(),
			mylib::date().set_timezone(_info.timezone).iso_string(),
			_device->ip(), _device->port()
		);
	}
	catch (const std::exception& e) {
		log_error(_device->serial_number(), "[device_manager][%lld] database error: %s", _device->conn_id(), e.what());
		_device->logout();
		return false;
	}

	// Start device thread
	_running = true;
	_connected = true;
	_thr = std::thread(&device_manager::process, this);
	return true;
}

bool device_manager::logout() {

	try {
		_device->logout();
		_connected = false;
		return true;
	}
	catch (const std::exception& e) {
		log_error(_device->serial_number(), "[device_manager][%lld] logout() failed: %s", _device->conn_id(), e.what());
		_connected = false;
		return false;
	}

}

void device_manager::reload(std::string device_sn, std::string device_id, std::string device_ip, unsigned short device_port) {

	// compare
	if (_device->serial_number() == device_sn) {
		log_info(_device->serial_number(), "[device_manager][%lld] reload(%s, %s, %s, %u) skipping, because it is same device\n",
				 _device->conn_id(),
				 device_sn.c_str(),
				 device_id.c_str(),
				 device_ip.c_str(),
				 device_port);
		return;
	}

	_new_device = std::make_shared<device>(_device->conn_id(), device_sn, device_id, device_ip, device_port);
}

bool device_manager::is_running() {
	std::lock_guard<std::mutex> lock(_mtx);
	return _running;
}

bool device_manager::sleep_for(std::chrono::milliseconds t) {
	std::unique_lock<std::mutex> lock(_mtx);
	if(_running) {
		_cv.wait_for(lock, t);
	}
	return _running;
}

void device_manager::process() {
	
	// sleep time must vary between 10-30 seconds
	std::chrono::seconds wait_time(10 + random() % 20);
	log_info(_device->serial_number(), "[device_manager][%lld] update period %llu seconds", _device->conn_id(), wait_time.count());

	while (is_running()) {

		this->check_device_reload();
		
		try {
			if(!db::instance().update_last_request_time(
				_device->serial_number(),
				mylib::date().set_timezone(_info.timezone).iso_string(),
				_device->ip(),
				_device->port()
				)) {
				log_info(_device->serial_number(), "[device_manager][%lld] could't make update in database. trying to refresh the cache", _device->conn_id());
				
				// returns false if stopping is requested
				if(!sleep_for(std::chrono::seconds(5))) {
					break;
				}

				try {
					db::instance().get_device_info(_device->serial_number(), _info);
					continue;
				}
				catch(...) {
					log_info(_device->serial_number(), "[device_manager][%lld] device not found in database. loggin out", _device->conn_id());
					try {_device->reboot();}
					catch(...){}
					break;
				}
			}
		}
		catch (const std::exception& e) {
			log_error(_device->serial_number(), "[device_manager][%lld] update_last_request_time() failed: %s", _device->conn_id(), e.what());
		}

		// always sync device's timezone
		try {
			_device->set_timezone(_info.timezone);
		}
		catch (...) { }

		// returns false when connection is lost
		if(!sync_records() || !process_commands()) {
			log_info(_device->serial_number(), "[device_manager][%lld] connection lost. stopping process...", _device->conn_id());
			_device->logout();
			_connected = false;
			break;
		}
		
		// returns false if stopping is requested
		if(!sleep_for(wait_time)){
			break;
		}
	}

	log_info(_device->serial_number(), "[device_manager][%lld] process stopped", _device->conn_id());
}

bool device_manager::sync_records() {

	if (_info.last_inout_time.empty()) {
		_info.last_inout_time = "2010-01-01T00:00:00Z";
	}
	
	auto start = mylib::date(_info.last_inout_time)
		.set_timezone(_info.timezone);
	auto end = mylib::date()
		.set_timezone(_info.timezone);
	auto request_date = end.iso_string();

	mylib::date next_event_date = start;
	std::list<device_event> records;

	// get records from device
	try {
		_device->get_records(start, end, records, next_event_date);
		if (this->check_device_reload()) {
			return true;
		}
	}
	catch (const std::exception& e) {
		
		log_error(_device->serial_number(), "[device_manager][%lld] sync_records(%s, %s) -> %s",
				  _device->conn_id(),
				  start.iso_string().c_str(),
				  end.iso_string().c_str(),
				  e.what());

		if (_connected && !_device->logout()) {
			log_error(_device->serial_number(), "[device_manager][%lld] sync_records: couldn't logout device", _device->conn_id());
			return true;
		};
		return false;
	}

	// write records to database
	try {
		for (auto& r : records) {
			if (_info.mode == device_mode::check_in) {
				r.attendance = attendance::ATTENDANCE_CHECKIN;
			}
			else if (_info.mode == device_mode::check_out) {
				r.attendance = attendance::ATTENDANCE_CHECKOUT;
			}
			r.request_date = request_date;
			db::instance().write_record(_device->serial_number(), r);
		}

		if (start != next_event_date) {
			_info.last_inout_time = next_event_date.iso_string();
			db::instance().update_last_inout_time(_device->serial_number(), _info.last_inout_time);
		}

	}
	catch (const std::exception& e) {
		log_error(_device->serial_number(), "[device_manager][%lld] sync_records(%s, %s)::db -> %s", 
			_device->conn_id(),
			start.iso_string().c_str(),
			end.iso_string().c_str(),
			e.what());
	}

	return true;
}

bool device_manager::process_commands() {

	std::list<device_command> cmds;
	try {
		db::instance().get_commands(_device->serial_number(), cmds);
	}
	catch (const std::exception& e) {
		log_error(_device->serial_number(), "[device_manager][%lld] get_commands() failed: %s", _device->conn_id(), e.what());
		return true;
	}
	
	if (cmds.size() == 0) {
		return true;
	}

	if (this->check_device_reload()) {
		return true;
	}

	log_info(_device->serial_number(), "[device_manager][%lld] processing %u commands\n", _device->conn_id(), cmds.size());
	auto now = mylib::date().set_timezone(_info.timezone).iso_string();
	std::list<command_status> statuses;
	bool connection_lost = false;

	for (auto& c : cmds) {

		if (this->check_device_reload()) {
			break;
		}

		command_status s;
		s.cmd_id = c.id;
		s.transfer_time = now;
		std::string short_cmd = c.data.substr(0, std::min(100, (int)c.data.length()));
		try {
			log_info(_device->serial_number(), "[device_manager][%lld] COMMAND-STARTED: \"%s\"", _device->conn_id(), short_cmd.c_str());
			process_single_command(c.data);
			s.success = true;
			log_info(_device->serial_number(), "[device_manager][%lld] COMMAND-SUCCESS: \"%s\"\n", _device->conn_id(), short_cmd.c_str());
		}
		catch (const std::exception& e) {
			s.success = false;
			s.error = e.what();
			log_info(_device->serial_number(), "[device_manager][%lld] COMMAND-FAILED: \"%s\"\n%s\n", _device->conn_id(), short_cmd.c_str(), e.what());
		}
		s.over_time = mylib::date().set_timezone(_info.timezone).iso_string();

		// Logout device if network fails
		if (s.error.find("NET_DVR_NETWORK_RECV_TIMEOUT") != std::string::npos ||
			s.error.find("NET_DVR_NETWORK_SEND_ERROR") != std::string::npos ||
			s.error.find("NET_DVR_NETWORK_FAIL_CONNECT") != std::string::npos ||
			s.error.find("NET_DVR_NETWORK_RECV_ERROR") != std::string::npos) {

			if (c.data.find("SET AUTOREGISTER") != std::string::npos) { // assume success for this command
				s.success = true;
				statuses.push_back(s);
			}
			log_error(_device->serial_number(), "[device_manager][%lld] logging out device because of %s", _device->conn_id(), s.error.c_str());

			if(_device->logout()) {
				connection_lost = true;
			}
			else {
				log_error(_device->serial_number(), "[device_manager][%lld] process_manager: couldn't logout device", _device->conn_id());
			}
			break;
		}

		statuses.push_back(s);
	}
	log_info(_device->serial_number(), "[device_manager][%lld] processed %u/%u commands\n", _device->conn_id(), statuses.size(), cmds.size());

	// write statues to db
	for (auto& s : statuses) {
		try {
			db::instance().write_command_status(s);
		}
		catch (const std::exception& e) {
			log_error(_device->serial_number(),
				"[device_manager][%lld] failed to write command status: cmd_id=%lld, success=%d, error=%s. exception=%s", 
				_device->conn_id(),
				s.cmd_id, s.success, s.error.c_str(), e.what());
		}
	}

	return !connection_lost;
}

void device_manager::process_single_command(const std::string& content) {
	
	std::string cmd;
	std::map<std::string, std::string> params;
	if (parse_command(content, cmd, params)) {

		if (cmd == "CLEAR DATA") {
			_device->clear_data();
		}
		else if (cmd == "SET DEVICE_TIME_ZONE") {
			long timezone = std::atol(at(params, "OFFSET").c_str());
			_device->set_timezone(timezone);
			_info.timezone = timezone;
		}
		else if (cmd == "SET NETWORK") {
			auto enable_dhcp = std::atol(at(params, "DHCP_ENABLED").c_str());
			auto& ip = at(params, "IP");
			auto& mask = at(params, "SUBNET_MASK");
			auto& gateway = at(params, "DEFAULT_GATEWAY");
			_device->set_network(enable_dhcp, ip, mask, gateway);
		}
		else if (cmd == "SET AUTOREGISTER") {
			auto enable = std::atol(at(params, "ENABLED").c_str());
			auto device_name = at(params, "DEVICE_NAME");
			auto server_ip = at(params, "SERVER_IP");
			auto server_port = std::atol(at(params, "SERVER_PORT").c_str());
			auto ehome_key = at(params, "EHOME_KEY");
			_device->set_isup(enable, device_name, server_ip, server_port, ehome_key);
		}
		else if (cmd == "SET ATTENDANCE") {

			if (params.count("MODE")) {
				auto mode = at(params, "MODE");
				auto enabled = std::atol(at(params, "ENABLED").c_str());
				_device->set_attendance_mode(mode, enabled);
			}
			else if (params.count("STATUS")) {
				auto status = at(params, "STATUS");
				auto label = at(params, "LABEL");
				auto enabled = std::atol(at(params, "ENABLED").c_str());
				_device->set_attendance_status(status, label, enabled);
			}

		}
		else if (cmd == "RECONNECT") {
			_device->logout();
		}
		else if (cmd == "REBOOT") {
			try {
				_device->reboot();
			}
			catch (const std::exception& e) {}
		}
		else if (cmd == "DATA USER") {
			auto& user_id = at(params, "PIN");
			auto& user_name = at(params, "Name");
			auto& password = at(params, "Passwd");
			auto& card_raw = at(params, "Card");
			auto is_admin = std::atol(at(params, "Pri").c_str());
			_device->add_user(user_id, user_name, password, is_admin);

			std::string card;
			if (parse_card(card_raw, card)) {
				_device->add_card(user_id, card);
			}
		}
		else if (cmd == "DATA FACE") {
			auto& user_id = at(params, "PIN");
			auto& face_id = at(params, "FACE_ID");
			std::string url = "http://" + config::public_ip() + ":" + std::to_string(config::http_server_port()) + "/face?id=" + face_id;
			_device->set_face_url(user_id, url);
		}
		else if (cmd == "DATA CARD") {
			auto& user_id = at(params, "PIN");
			auto& card_raw = at(params, "Card");
			std::string card;
			if (parse_card(card_raw, card)) {
				_device->add_card(user_id, card);
			}
		}
		else if (cmd == "DATA DEL_USER") {
			auto& user_id = at(params, "PIN");
			_device->del_user(user_id);
		}
		else if (cmd == "DATA DEL_FACE") {
			auto& user_id = at(params, "PIN");
			_device->del_facedata(user_id);
		}
		else if (cmd == "DATA DEL_CARD") {
			if (params.count("Card")) {
				auto& card_raw = at(params, "Card");
				std::string card;
				if (parse_card(card_raw, card)) {
					_device->del_card(card);
				}
			}
			else if (params.count("PIN")) {
				auto& user_id = at(params, "PIN");
				_device->del_user_cards(user_id);
			}
			else {
				throw std::runtime_error("No param found. (CARD or PIN)");
			}
		}
		else if (cmd == "SYNC CARD") {
			auto& user_id = at(params, "PIN");
			auto employee_id = std::atoll(at(params, "EMPLOYEE_ID").c_str());
			std::list<std::string> cards;
			_device->get_cards(user_id, cards);
			if (cards.empty()) {
				throw std::runtime_error("No card found");
			}
			db::instance().delete_cards(employee_id, _info.company_id);
			db::instance().write_card_data(employee_id, _info.company_id, _info.card_type_id, cards.front());
		}
		else if (cmd == "SYNC FACE") {
			auto& user_id = at(params, "PIN");
			auto employee_id = std::atoll(at(params, "EMPLOYEE_ID").c_str());
			std::string face_url;
			_device->get_face_url(user_id, face_url);
			auto pos = face_url.find("id=");
			std::string image_id;
			if (pos != std::string::npos) {
				image_id = face_url.substr(pos + 3);
			}
			auto image_path = find_image(image_id);
			if (image_path.empty()) {
				throw std::runtime_error("couldn't find image file with id = " + image_id);
			}
			std::ifstream file(image_path, std::ios::binary);
			if (!file.is_open()) {
				remove_image(image_id);
				throw std::runtime_error("couldn't open file " + image_path + ". face_url=" + face_url);
			}

			file.seekg(0, std::ios::end);
			auto size = file.tellg();
			file.seekg(0, std::ios::beg);
			std::vector<char> face_data(size);
			file.read(face_data.data(), size);
			file.close();
			// remove_image(image_id);
			std::remove(image_path.c_str());
			if (face_data.size()) {
				db::instance().write_face_data(employee_id, std::move(face_data));
			}
		}
		else if (cmd == "SYNC RECORDS") {
			auto start = mylib::date(2000).set_timezone(_info.timezone);
			auto end = mylib::date().set_timezone(_info.timezone);
			if (params.count("START")) {
				start = mylib::date(params.at("START")).set_timezone(_info.timezone);
			}
			if (params.count("END")) {
				end = mylib::date(params.at("END")).set_timezone(_info.timezone);
			}
			std::list<device_event> records;
			_device->get_records(start, end, records, start);
			if (records.empty()) {
				throw std::runtime_error("No records found");
			}

			auto now = mylib::date().iso_string();

			// write to database
			for (auto& r : records) {
				if (_info.mode == device_mode::check_in) {
					r.attendance = attendance::ATTENDANCE_CHECKIN;
				}
				else if (_info.mode == device_mode::check_out) {
					r.attendance = attendance::ATTENDANCE_CHECKOUT;
				}
				r.request_date = now;
				db::instance().write_record(_device->serial_number(), r);
			}
		}
		else if (cmd == "DATA FP") {
			auto& user_id = at(params, "PIN");
			int index = 1;
			if (params.count("INDEX")) {
				index = (int) std::atol(at(params, "INDEX").c_str());
			}
			auto data = at(params, "TMP");
			_device->set_fingerprint(user_id, index, data);
		}
		else if (cmd == "DATA DEL_FP") {
			auto& user_id = at(params, "PIN");
			if (params.count("INDEX")) {
				long index = std::atol(params.at("INDEX").c_str());
				_device->del_fingerprint(user_id, index);
			}
			else {
				_device->del_fingerprint(user_id, 0);
			}
		}
		else if (cmd == "SYNC FP") {
			auto& user_id = at(params, "PIN");
			auto employee_id = std::atoll(at(params, "EMPLOYEE_ID").c_str());
			std::list<std::pair<std::string, int>> fingerprints;
			_device->get_fingerprints(user_id, fingerprints);

			if (fingerprints.empty()) {
				throw std::runtime_error("No fingerprint found");
			}

			for (auto& pair : fingerprints) {
				db::instance().write_fingerprint_data(employee_id, pair.second, base64_decode(pair.first));
			}
		}
		else if (cmd == "GET RECORDS") {
			auto start = mylib::date(2000).set_timezone(_info.timezone);
			auto end = mylib::date().set_timezone(_info.timezone);
			if (params.count("START")) {
				start = mylib::date(params.at("START")).set_timezone(_info.timezone);
			}
			if (params.count("END")) {
				end = mylib::date(params.at("END")).set_timezone(_info.timezone);
			}
			std::list<device_event> records;
			_device->get_records(start, end, records, start);
			if (records.empty()) {
				throw std::runtime_error("No records found");
			}

			save_records_as_csv(records);
		}
		else if (cmd == "DUMP USERS") {
			std::list<device_user> users;
			_device->get_users(users);
			save_users_as_csv(users);
		}
		else if (cmd == "DUMP CARDS") {
			auto& user_id = at(params, "PIN");
			std::list<std::string> cards;
			_device->get_cards(user_id, cards);
			save_cards_as_csv(user_id, cards);
		}
		else if (cmd == "DUMP FACE") {
			auto face_id = std::atol(at(params, "FACE_ID").c_str());

			// get facedata from database
			std::vector<char> face_data;
			db::instance().get_face_data(face_id, face_data);
			save_facedata(face_id, face_data);
		}
		else {
			log_error(_device->serial_number(), "[device_manager] Unknown command: %s", cmd.c_str());
			throw std::runtime_error("Unknown command");
		}
	}
	else {
		log_error(_device->serial_number(), "[device_manager] Invalid command format: %s", content.c_str());
		throw std::runtime_error("Invalid command format: " + content);
	}

}

bool device_manager::parse_command(const std::string& content, std::string& cmd, std::map<std::string, std::string>& params) {
	std::smatch match;
	if( !std::regex_search(content, match, std::regex("[A-Z_]+ [A-Z_]+ ")) ) {

		if( std::regex_search(content, match, std::regex("[A-Z_]+ [A-Z_]+")) ) {
			cmd = match[0];
			return true;
		}
		else if( std::regex_search(content, match, std::regex("[A-Z_]+")) ) {
			cmd = match[0];
			return true;
		}
		return false;
	}
	cmd = match[0];
	cmd.pop_back(); // trim last whitespace
	if( match.suffix().length() == 0 ) {
		return true;
	}

	std::string args = std::string(match.suffix()) + "\t";
	while( std::regex_search(args, match, std::regex("\t")) ) {
		std::string pair = match.prefix();
		std::size_t pos = pair.find('=');
		std::string key = pair.substr(0, pos);
		std::string value = pair.substr(pos + 1);
		params.insert({ key, value });
		args = match.suffix();
	}
	return true;
}

bool device_manager::parse_card(const std::string& raw, std::string& card_no) {
	if (raw.size() > 2 && raw.front() == '[' && raw.back() == ']') {
		card_no = raw.substr(1, raw.size() - 2);
		std::string empty_card(card_no.size(), '0');
		if (card_no == empty_card) {
			card_no.clear();
		}
	}
	return card_no.size();
}

const std::string& device_manager::at(const std::map<std::string, std::string>& map, const std::string& key) {
	try {
		return map.at(key);
	}
	catch(...) {
		throw std::runtime_error("\"" + key + "\" is required param");
	}
}

void device_manager::save_records_as_csv(const std::list<device_event>& records) {

	std::string filename = mylib::date().set_timezone(_info.timezone).iso_string().substr(0, 10) + ".csv";
	std::string dir = config::records_directory() + std::to_string(_info.company_id) + "/" + _device->serial_number() + "/";
	std::filesystem::create_directories(dir);
	std::ofstream file(dir + filename, std::ios_base::trunc);
	if (!file.is_open()) {
		log_error(_device->serial_number(), "save_records_as_csv() failed: couldn't open file \"%s%s\"", dir.c_str(), filename.c_str());
		return;
	}

	if( file.tellp() == 0 ) {
		file << "RowID, PIN, EventDate, Verify" << std::endl;
	}

	for( auto& r : records ) {
		file << r.row_id << ", " << r.user_id << ", " << r.event_date << ", " << r.unlock_method << std::endl;
	}

	file.close();
}

void device_manager::save_users_as_csv(const std::list<device_user>& users) {
	std::string filename = mylib::date().set_timezone(_info.timezone).iso_string().substr(0, 10) + ".csv";
	std::string dir = config::records_directory() + std::to_string(_info.company_id) + "/" + _device->serial_number() + "/users/";
	std::filesystem::create_directories(dir);
	std::ofstream file(dir + filename, std::ios_base::trunc);
	if (!file.is_open()) {
		log_error(_device->serial_number(), "save_users_as_csv() failed: couldn't open file \"%s%s\"", dir.c_str(), filename.c_str());
		return;
	}

	if( file.tellp() == 0 ) {
		file << "PIN, Name, Password" << std::endl;
	}

	for( auto& u : users ) {
		file << u.user_id << ", " << u.name << ", " << u.password << std::endl;
	}

	file.close();
}

void device_manager::save_cards_as_csv(const std::string& user_id, const std::list<std::string>& cards) {
	std::string filename = user_id + "-" + mylib::date().set_timezone(_info.timezone).iso_string().substr(0, 10) + ".csv";
	std::string dir = config::records_directory() + std::to_string(_info.company_id) + "/" + _device->serial_number() + "/cards/";
	std::filesystem::create_directories(dir);
	std::ofstream file(dir + filename, std::ios_base::trunc);
	if (!file.is_open()) {
		log_error(_device->serial_number(), "save_users_as_csv() failed: couldn't open file \"%s%s\"", dir.c_str(), filename.c_str());
		return;
	}

	if( file.tellp() == 0 ) {
		file << "PIN, CardNo" << std::endl;
	}

	for( auto& c : cards ) {
		file << user_id << "," << c << std::endl;
	}

	file.close();
}

void device_manager::save_facedata(int face_id, const std::vector<char>& data) {
	std::string filename = std::to_string(face_id) + ".jpg";
	std::string dir = config::records_directory() + std::to_string(_info.company_id) + "/" + _device->serial_number() + "/faces/";
	std::filesystem::create_directories(dir);
	std::ofstream file(dir + filename, std::ios_base::trunc | std::ios_base::binary);
	if (!file.is_open()) {
		log_error(_device->serial_number(), "save_facedata() failed: couldn't open file \"%s%s\"", dir.c_str(), filename.c_str());
		return;
	}
	file.write(data.data(), data.size());
	file.close();
}

void device_manager::save_facedata_as_base64(int face_id, const std::string& data) {
	std::string filename = std::to_string(face_id) + ".txt";
	std::string dir = config::records_directory() + std::to_string(_info.company_id) + "/" + _device->serial_number() + "/faces/";
	std::filesystem::create_directories(dir);
	std::ofstream file(dir + filename, std::ios_base::trunc | std::ios_base::binary);
	if (!file.is_open()) {
		log_error(_device->serial_number(), "save_facedata() failed: couldn't open file \"%s%s\"", dir.c_str(), filename.c_str());
		return;
	}
	file.write(data.data(), data.size());
	file.close();
}

std::string device_manager::find_image(const std::string& image_id) {
	std::string image_path = config::temp_images_directory() + image_id + ".jpg";
	if (std::filesystem::exists(image_path)) {
		return image_path;
	}
	return "";
}

void device_manager::remove_image(const std::string& image_id) {
	std::string image_path = config::temp_images_directory() + image_id + ".jpg";
	if (std::filesystem::exists(image_path)) {
		std::remove(image_path.c_str());
	}
}

bool device_manager::check_device_reload() {

	if (_new_device) {
		_device = std::move(_new_device);
		return true;
	}
	return false;
}