#include "cms_server.hpp"
#define CMS "CMS"

BOOL CALLBACK device_register_cb(LONG conn_id, DWORD data_type, void* out_buf, DWORD out_len, void* in_buf, DWORD in_len, void* ctx) {

	auto pthis = reinterpret_cast<cms_server*>(ctx);

	// when device off, no out_buf is provided, out_len is zero
	if (data_type == ENUM_DEV_OFF) {
		log_info(CMS, "REGISTER_DEVICE(type = ENUM_DEV_OFF, conn_id = %ld)", conn_id);
		pthis->device_off(conn_id);
		return TRUE;
	}

	if (out_len < sizeof(NET_EHOME_DEV_REG_INFO_V12)) {
		log_error(CMS, "REGISTER_DEVICE(type = %u, %u < NET_EHOME_DEV_REG_INFO_V12)", data_type, out_len);
		return FALSE;
	}
	
	auto dev = (NET_EHOME_DEV_REG_INFO_V12*)out_buf;
	if (in_buf != nullptr) {
		std::memset(in_buf, 0, in_len);
	}

	if (data_type == ENUM_DEV_AUTH) {
		std::memcpy(in_buf, config::ehome_key().data(), config::ehome_key().size());
		log_info(CMS,
				 "REGISTER_DEVICE(type = ENUM_DEV_AUTH, dev_sn = %s, dev_id = %s)",
				 (const char*)dev->struRegInfo.sDeviceSerial,
				 (const char*)dev->struRegInfo.byDeviceID);
	}
	else if (data_type == ENUM_DEV_SESSIONKEY) {
		NET_EHOME_DEV_SESSIONKEY key = { 0 };
		std::memcpy(key.sDeviceID, dev->struRegInfo.byDeviceID, MAX_DEVICE_ID_LEN);
		std::memcpy(key.sSessionKey, dev->struRegInfo.bySessionKey, MAX_MASTER_KEY_LEN);
		NET_ECMS_SetDeviceSessionKey(&key);
		log_info(CMS,
				 "REGISTER_DEVICE(type = ENUM_DEV_SESSIONKEY, dev_sn = %s, dev_id = %s)",
				 (const char*)dev->struRegInfo.sDeviceSerial,
				 (const char*)dev->struRegInfo.byDeviceID);
	}
	else if (data_type == ENUM_DEV_SESSIONKEY_ERROR) {
		log_error(CMS,
				  "REGISTER_DEVICE(type = ENUM_DEV_SESSIONKEY_ERROR, dev_sn = %s, dev_id = %s)",
				  (const char*)dev->struRegInfo.sDeviceSerial,
				  (const char*)dev->struRegInfo.byDeviceID);
		pthis->device_off(conn_id);
	}
	else if (data_type == ENUM_DEV_DAS_EHOMEKEY_ERROR) {
		log_error(CMS,
				  "REGISTER_DEVICE(type = ENUM_DEV_DAS_EHOMEKEY_ERROR, dev_sn = %s, dev_id = %s)",
				  (const char*)dev->struRegInfo.sDeviceSerial,
				  (const char*)dev->struRegInfo.byDeviceID);
	}
	else if (data_type == ENUM_DEV_DAS_REQ) {
		std::string data = nlohmann::json({
			{"Type", "DAS"},
			{"DasInfo", {
				{"Address", config::public_ip()},
				{"ServerID", pthis->server_id()},
				{"Port", config::cms_server_port()},
				{"UdpPort", 0}
			}}
		}).dump();
		std::memcpy(in_buf, data.data(), data.size());

		log_info(CMS,
				 "REGISTER_DEVICE(type = ENUM_DEV_DAS_REQ, dev_sn = %s, dev_id = %s)",
				 (const char*)dev->struRegInfo.sDeviceSerial,
				 (const char*)dev->struRegInfo.byDeviceID);
	}
	else if (data_type == ENUM_DEV_DAS_REREGISTER) {
		auto serv = (NET_EHOME_SERVER_INFO_V50*)in_buf;
		serv->dwSize = sizeof(NET_EHOME_SERVER_INFO_V50);

		auto alarm = pthis->next_alarm_server();
		auto storage = pthis->next_storage_server();

		std::strcpy(serv->struTCPAlarmSever.szIP, alarm.first.c_str());
		serv->struTCPAlarmSever.wPort = alarm.second;
		serv->dwAlarmServerType = 2; // mqqt

		std::strcpy(serv->struPictureSever.szIP, storage.first.c_str());
		serv->struPictureSever.wPort = storage.second;
		serv->dwPicServerType = 3; // kms

		log_info(CMS,
				 "REGISTER_DEVICE(type = ENUM_DEV_DAS_REREGISTER, conn_id = %ld, dev_sn = %s, dev_id = %s, alarm = %s:%u, ss = %s:%u)",
				 conn_id,
				 (const char*)dev->struRegInfo.sDeviceSerial,
				 (const char*)dev->struRegInfo.byDeviceID,
				 alarm.first.c_str(), alarm.second,
				 storage.first.c_str(), storage.second);

		pthis->device_on(conn_id, dev);
	}
	else if (data_type == ENUM_DEV_ON) {
		auto serv = (NET_EHOME_SERVER_INFO_V50*)in_buf;
		serv->dwSize = sizeof(NET_EHOME_SERVER_INFO_V50);

		auto alarm = pthis->next_alarm_server();
		auto storage = pthis->next_storage_server();

		std::strcpy(serv->struTCPAlarmSever.szIP, alarm.first.c_str());
		serv->struTCPAlarmSever.wPort = alarm.second;
		serv->dwAlarmServerType = 2; // mqqt

		std::strcpy(serv->struPictureSever.szIP, storage.first.c_str());
		serv->struPictureSever.wPort = storage.second;
		serv->dwPicServerType = 3; // kms

		log_info(CMS,
				 "REGISTER_DEVICE(type = ENUM_DEV_ON, conn_id = %ld, dev_sn = %s, dev_id = %s, dev_ip = %s, dev_port = %u, alarm = %s:%u, ss = %s:%u)",
				 conn_id,
				 (const char*)dev->struRegInfo.sDeviceSerial,
				 (const char*)dev->struRegInfo.byDeviceID,
				 (const char*)dev->struRegInfo.struDevAdd.szIP,
				 dev->struRegInfo.struDevAdd.wPort,
				 alarm.first.c_str(), alarm.second,
				 storage.first.c_str(), storage.second);

		pthis->device_on(conn_id, dev);
	}
	return TRUE;
}

bool cms_server::_init = false;

void cms_server::init_sdk() {
	if (!_init) {
		NET_ECMS_Init();

		if (config::enable_sdk_logs()) {
			NET_ECMS_SetLogToFile(3, (char*)(config::logs_directory() + "sdk_cms.log").c_str(), TRUE);
		}
		
		_init = true;
	}
}

void cms_server::destroy_sdk() {
	if (_init) {
		NET_ECMS_Fini();
		_init = false;
	}
}

bool cms_server::start() {
	
	if (_handle != -1) {
		return false;
	}

	srand(time(nullptr));

	NET_EHOME_CMS_LISTEN_PARAM param = { 0 };
	std::strcpy(param.struAddress.szIP, "0.0.0.0");
	param.struAddress.wPort = config::cms_server_port();
	param.fnCB = device_register_cb;
	param.dwKeepAliveSec = 20;
	param.dwTimeOutCount = 5;
	param.pUserData = this;
	_handle = NET_ECMS_StartListen(&param);
	if (_handle == -1) {
		return false;
	}
	log_info(CMS, "cms server listening on port %u", config::cms_server_port());

	// Run garbage collector
	_cmd_handler_running = true;
	_thr_cmd = std::thread(&cms_server::command_handler, this);
	return true;
}

void cms_server::stop() {
	if (_handle != -1) {
		NET_ECMS_StopListen(_handle);
		_handle = -1;

		_cmd_handler_running = false;
		_cv_cmd.notify_all();
		if (_thr_cmd.joinable()) {
			_thr_cmd.join();
		}

		log_info(CMS, "server stopped");
	}
}

void cms_server::device_on(long conn_id, NET_EHOME_DEV_REG_INFO_V12* dev) {

	if (this->try_reload(conn_id, dev)) {
		return;
	}

	std::lock_guard<std::mutex> lock(_mtx_cmd);
	_cmd_queue.push_back({ command_type::device_on, conn_id, *dev });
	_cv_cmd.notify_one();
}

void cms_server::device_off(long conn_id) {
	std::lock_guard<std::mutex> lock(_mtx_cmd);
	_cmd_queue.push_back({ command_type::device_off, conn_id, {} });
	_cv_cmd.notify_one();
}

const std::pair<std::string, uint16_t>& cms_server::next_alarm_server() {
	return config::alarm_servers().at(_next_alarm_server++ % config::alarm_servers().size());
}

const std::pair<std::string, uint16_t>& cms_server::next_storage_server() {
	return config::storage_servers().at(_next_storage_server++ % config::storage_servers().size());
}

void cms_server::log_online_devices(const std::map<long, std::unique_ptr<device_manager>>& devices) {

	if (config::enable_debug_logs()) {
		std::stringstream ss;

		ss << "Online devices count: " << devices.size() << std::endl;
		for (auto& d : devices) {
			ss << "\t" << d.first << " => " << d.second->serial_number() << std::endl;
		}
		log_debug(CMS, "%s", ss.str().c_str());
	}
}


std::string cms_server::server_id() {
	return "das_cms_" + std::to_string(rand());
}

bool cms_server::dequeue_commands(std::list<command>& cmds) {

	std::unique_lock<std::mutex> lock(_mtx_cmd);

	if (_cmd_queue.empty()) {
		_cv_cmd.wait_for(lock, std::chrono::milliseconds(1000));
	}

	if (!_cmd_handler_running) {
		return false;
	}

	if (_cmd_queue.size()) {
		cmds = std::move(_cmd_queue);
		_cmd_queue.clear();
	}
	else {
		cmds.clear();
	}

	return true;
}

void cms_server::command_handler() {

	std::list<command> cmds;
	while (this->dequeue_commands(cmds)) {
		
		for (auto& cmd : cmds) {

			if (cmd.type == command_type::device_on) {

				if (this->try_reload(cmd.conn_id, &cmd.dev)) {
					continue;
				}
				
				long conn_id = cmd.conn_id;
				const char* dev_id = (const char*)cmd.dev.struRegInfo.byDeviceID;
				const char* dev_sn = (const char*)cmd.dev.struRegInfo.sDeviceSerial;
				const char* dev_ip = (const char*)cmd.dev.struRegInfo.struDevAdd.szIP;
				unsigned short dev_port = config::cms_server_port(); // request from Alex. cmd.dev.struRegInfo.struDevAdd.wPort; 

				auto manager = std::make_unique<device_manager>(conn_id, dev_sn, dev_id, dev_ip, dev_port);
				if (manager->run()) {

					ipc_server::instance().set_session_key(
						conn_id,
						cmd.dev.struRegInfo.byDeviceID, MAX_DEVICE_ID_LEN,
						cmd.dev.struRegInfo.bySessionKey, MAX_MASTER_KEY_LEN
					);

					std::lock_guard<std::mutex> lock(_mtx_devices);
					if (_devices.insert(std::make_pair(conn_id, std::move(manager))).second) {
						log_online_devices(_devices);
					}
					else {
						log_error(CMS, "device_on(conn_id = %d, sn = %s): failed to insert device manager to the list", conn_id, dev_sn);
					}
				}
			}
			else if (cmd.type == command_type::device_off) {
				ipc_server::instance().remove_session_key(cmd.conn_id);

				std::unique_lock<std::mutex> lock(_mtx_devices);
				auto it = _devices.find(cmd.conn_id);
				if (it != _devices.end()) {
					auto ptr = std::move(it->second);
					_devices.erase(it);
					log_online_devices(_devices);
					lock.unlock();
					// device_mananger::~destructor takes some time, so we hold
					// it in "ptr" until _mtx_devices unlocked.
				}
			}

		}

		// check inactive device managers
		std::list<std::unique_ptr<device_manager>> ptr_list;
		std::unique_lock<std::mutex> lock(_mtx_devices);
		for (auto it = _devices.begin(); it != _devices.end(); ) {
			if (it->second->connected()) {
				++it;
			}
			else {
				log_info(CMS, "removing inactive device manager: sn=%s, conn_id=%lld",
					it->second->serial_number().c_str(),
						 it->second->conn_id());

				// we need to hold "device managers" in "ptr_list",
				// because their destructors takes some to; but we need to release 
				// _mtx_devices asap!
				ptr_list.push_back(std::move(it->second));
				it = _devices.erase(it);
			}
		}
		lock.unlock();
	}

}

bool cms_server::try_reload(long conn_id, NET_EHOME_DEV_REG_INFO_V12* dev) {

	std::lock_guard<std::mutex> lock(_mtx_devices);
	auto it = _devices.find(conn_id);
	if (it == _devices.end()) {
		return false;
	}

	const char* dev_id = (const char*)dev->struRegInfo.byDeviceID;
	const char* dev_sn = (const char*)dev->struRegInfo.sDeviceSerial;
	const char* dev_ip = (const char*)dev->struRegInfo.struDevAdd.szIP;
	unsigned short dev_port = config::cms_server_port();
	it->second->reload(dev_id, dev_sn, dev_ip, dev_port);
	return true;
	
}