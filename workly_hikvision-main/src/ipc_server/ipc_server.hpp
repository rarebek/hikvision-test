#pragma once

#include <cstdint>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <list>
#include <cstring>
#include <unordered_map>
#include "../logger/logger.hpp"
#include "../config/config.hpp"
#include "ipc_client.hpp"

class ipc_server {
	ipc_server();
	~ipc_server();
public:
	static ipc_server& instance();


	void start(uint16_t listening_port);
	void stop();

	void set_session_key(
		int conn_id,
		const uint8_t* device_id,
		uint32_t device_id_len,
		const uint8_t* sess_key,
		uint32_t sess_key_len
	);

	void remove_session_key(int conn_id);


private:
	void event_loop(int efd, int sfd);
	void notify_one();
	void notify_reset();

	std::thread _thr;
	std::mutex _mtx;
	bool _running;
	int _notifiy_fd;
	int _wait_fd;
	std::unordered_map<int, std::vector<uint8_t>> _commands;
};

