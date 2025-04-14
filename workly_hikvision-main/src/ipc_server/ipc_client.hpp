#pragma once

#include <cstdint>
#include <vector>
#include <list>
#include <ctime>
#include <sys/unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <error.h>
#include <string.h>
#include "../logger/logger.hpp"
#include "../encoders/hex/hex.hpp"

class ipc_client {
public:
	ipc_client(int efd, int cfd);
	ipc_client(ipc_client&&);
	~ipc_client();

	void append(const std::vector<uint8_t>& data);

	bool read();
	bool write();

	bool tick();

private:
	int _efd;
	int _cfd;
	std::time_t _last_echo_send_time = 0;
	std::time_t _last_echo_recv_time = 0;
	int _echo_timeout_times = 0;
	

	std::vector<uint8_t> _input_buffer;
	std::list<std::vector<uint8_t>> _output_data;

	bool parse_input_data();
};