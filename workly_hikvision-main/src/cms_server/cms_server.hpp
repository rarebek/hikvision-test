#pragma once

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <map>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <list>
#include <thread>
#include <cstdlib>
#include "../device_manager/device_manager.hpp"
#include "../encoders/nlohmann/json.hpp"
#include "../encoders/hex/hex.hpp"
#include "../logger/logger.hpp"
#include "../config/config.hpp"
#include "../ipc_server/ipc_server.hpp"
#include "../../libisup/include/HCISUPCMS.h"

class cms_server {
public:

	static void init_sdk();
	static void destroy_sdk();

	bool start();
	void stop();

	void device_on(long conn_id, NET_EHOME_DEV_REG_INFO_V12* dev);
	void device_off(long conn_id);

	std::string server_id();

	const std::pair<std::string, uint16_t>& next_alarm_server();
	const std::pair<std::string, uint16_t>& next_storage_server();

private:
	static bool _init;
	long _handle = -1;
	int _next_alarm_server = 0;
	int _next_storage_server = 0;

private:
	enum class command_type {
		device_on, device_off
	};
	struct command {
		command_type type;
		long conn_id;
		NET_EHOME_DEV_REG_INFO_V12 dev;
	};

	std::list<command> _cmd_queue;
	std::thread _thr_cmd;
	std::mutex _mtx_cmd;
	std::condition_variable _cv_cmd;
	bool dequeue_commands(std::list<command>& cmds);

private:
	bool _cmd_handler_running = false;
	void command_handler();
	void log_online_devices(const std::map<long, std::unique_ptr<device_manager>>& devices);

private:
	std::mutex _mtx_devices;
	std::map<long, std::unique_ptr<device_manager>> _devices;
	bool try_reload(long conn_id, NET_EHOME_DEV_REG_INFO_V12* dev);


};