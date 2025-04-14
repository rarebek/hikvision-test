#pragma once

#include "../device/device.hpp"
#include "../date/date.hpp"
#include "../db/db_types.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <list>
#include <memory>
#include <map>

class device_manager {
public:
	device_manager(long conn_id, std::string device_sn, std::string device_id, std::string device_ip, unsigned short device_port);
	~device_manager();

	bool run();
	bool connected() const { return _connected; }
	bool logout();

	void reload(std::string device_sn, std::string device_id, std::string device_ip, unsigned short device_port);

	int64_t conn_id() { return _device->conn_id(); };
	const std::string& name() { return _device->name(); };
	const std::string& serial_number() { return _device->serial_number(); };
	const device_info& meta() { return _info; }

private:
	bool sleep_for(std::chrono::milliseconds t);
	bool is_running();

	void process();
	bool sync_records();

	bool process_commands();
	void process_single_command(const std::string& content);
	void save_records_as_csv(const std::list<device_event>& records);
	void save_users_as_csv(const std::list<device_user>& users);
	void save_cards_as_csv(const std::string& user_id, const std::list<std::string>& cards);
	void save_facedata(int face_id, const std::vector<char>& data);
	void save_facedata_as_base64(int face_id, const std::string& base64);
	static bool parse_command(const std::string& content, std::string& cmd, std::map<std::string, std::string>& params);
	static bool parse_card(const std::string& raw, std::string& card);
	static const std::string& at(const std::map<std::string, std::string>& map, const std::string& key);
	static std::string find_image(const std::string& image_id);
	static void remove_image(const std::string& image_id);

private:
	std::shared_ptr<device> _device;
	std::shared_ptr<device> _new_device;
	mylib::date _last_inout_time;
	std::thread _thr;
	std::mutex _mtx;
	std::condition_variable _cv;
	device_info _info;
	bool _running;
	bool _connected;

	bool check_device_reload();
};