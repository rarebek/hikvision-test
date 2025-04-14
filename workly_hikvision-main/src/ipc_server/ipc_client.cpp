#include "ipc_client.hpp"
#define MODULE "IPC"


ipc_client::ipc_client(int efd, int cfd) {
	_efd = efd;
	_cfd = cfd;
	_last_echo_send_time = 0;
	_last_echo_send_time = 0;

	if (fcntl(cfd, F_SETFL, O_NONBLOCK) == -1) {
		close(cfd);
		char error[255];
		sprintf(error, "fcntl() failed: %s", strerror(errno));
		throw std::runtime_error(error);
	}

	epoll_event ev;
	ev.data.fd = _cfd;
	ev.events = EPOLLERR | EPOLLIN;
	if (epoll_ctl(_efd, EPOLL_CTL_ADD, _cfd, &ev)) {
		close(cfd);
		char error[255];
		sprintf(error, "epoll_ctl(add) failed: %s", strerror(errno));
		throw std::runtime_error(error);
	}
}

ipc_client::ipc_client(ipc_client&& other) {
	_efd = other._efd;
	_cfd = other._cfd;
	_last_echo_recv_time = other._last_echo_recv_time;
	_last_echo_send_time = other._last_echo_send_time;
	_input_buffer = std::move(other._input_buffer);
	_output_data = std::move(other._output_data);
	other._cfd = -1;
	other._efd = -1;
}

ipc_client::~ipc_client() {
	if (_cfd != -1) {
		log_debug(MODULE, "[%d] ~ipc_client", _cfd);
		epoll_event ev;
		epoll_ctl(_efd, EPOLL_CTL_DEL, _cfd, &ev);
		shutdown(_cfd, SHUT_RDWR);
		close(_cfd);
		_cfd = -1;
	}
}

void ipc_client::append(const std::vector<uint8_t>& data) {
	_output_data.push_back(data);
}

bool ipc_client::read() {

	constexpr int rcv_buf_size = 1024;
	char rcv_buf[rcv_buf_size];

	while (true) {
		int bytes_read = ::read(_cfd, rcv_buf, rcv_buf_size);
		if (bytes_read == -1) {
			if (errno == EAGAIN) {
				return parse_input_data();
			}
			log_error(MODULE, "[%d] read failed: %s", _cfd, strerror(errno));
			return false;
		}

		if (bytes_read == 0) {
			log_info(MODULE, "[%d] read 0 bytes. connection closed", _cfd);
			return false;
		}

		_input_buffer.insert(_input_buffer.end(), (uint8_t*)rcv_buf, (uint8_t*)rcv_buf + bytes_read);
	}
}

bool ipc_client::write() {

	while (true) {
		
		if (_output_data.empty()) {
			return true;
		}

		std::vector<uint8_t>& data = _output_data.front();
		ssize_t bytes_written = ::write(_cfd, data.data(), data.size());
		if (bytes_written == -1) {
			if (errno == EAGAIN) {
				return true;
			}
			log_error(MODULE, "[%d] write failed: %s", _cfd, strerror(errno));
			return false;
		}

		if (bytes_written == 0) {
			log_info(MODULE, "[%d] write: connection closed", _cfd);
			return false;
		}

		if (bytes_written == data.size()) {
			_output_data.pop_front();
		}
		else {
			data.erase(data.begin(), data.begin() + bytes_written);
		}

	}

}

bool ipc_client::tick() {

	epoll_event ev;
	ev.data.fd = _cfd;
	ev.events = EPOLLERR | EPOLLIN;

	std::time_t now = std::time(nullptr);
	std::time_t time_passed = now - _last_echo_send_time;
	if (time_passed > 30) {
		if (_last_echo_recv_time < _last_echo_send_time) {
			if (++_echo_timeout_times >= 3) {
				log_error(MODULE, "[%d] echo response timeout", _cfd);
				return false;
			}
		}

		// schedule new echo request
		_output_data.push_back({ 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x02 });
		_last_echo_send_time = now;
	}

	if (_output_data.size()) {
		ev.events |= EPOLLOUT;
	}

	if (epoll_ctl(_efd, EPOLL_CTL_MOD, _cfd, &ev) == -1) {
		log_error(MODULE, "[%d] epoll_ctl() failed: %s", _cfd, strerror(errno));
		return false;
	}

	return true;
}

bool ipc_client::parse_input_data() {

	int skipped_bytes = 0;
	
	while (true) {

		auto& data = _input_buffer;

		if (data.size() < 8) {
			return true;
		}

		// check sync code
		if (data[0] != 0xFF || data[1] != 0xFF || data[2] != 0xFF || data[3] != 0xFF) {

			if (++skipped_bytes >= 20) {
				log_error(MODULE, "[%d] invalid sync code. destroying connection", _cfd);
				return false;
			}

			log_error(MODULE, "[%d] invalid sync code. skipping 1 byte", _cfd);
			data.erase(data.begin());
			continue;
		}

		// check data type
		if (data[4] != 0x00 || data[5] != 0x00 || data[6] != 0x00) {
			log_error(MODULE, "[%d] invalid data type", _cfd);
			return false;
		}

		if (data[7] == 0x02) {
			// server echo response
			_last_echo_recv_time = std::time(nullptr);
			_echo_timeout_times = 0;
			data.erase(data.begin(), data.begin() + 8);
		}
		else if (data[7] == 0x03) {
			// client echo request: send response
			_output_data.push_back(std::vector<uint8_t>(data.begin(), data.begin() + 8));
			data.erase(data.begin(), data.begin() + 8);
		}
		else {
			log_error(MODULE, "[%d] unsupported data type: %s", _cfd, hex_encode(data).c_str());
			return false;
		}
	}

	return true;
}
