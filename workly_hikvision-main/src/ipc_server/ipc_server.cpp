#include "ipc_server.hpp"

#define MODULE "IPC"

ipc_server::ipc_server() {
	_running = false;
	_wait_fd = -1;
	_notifiy_fd = -1;
}

ipc_server::~ipc_server() {
	stop();
}

ipc_server& ipc_server::instance() {
	static ipc_server _instance;
	return _instance;
}


void ipc_server::start(uint16_t listening_port) {

	std::lock_guard<std::mutex> lock(_mtx);
	if (_running) {
		throw std::runtime_error("already running");
	}

	int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sfd == -1) {
		char error[255];
		sprintf(error, "socket() failed: %s", strerror(errno));
		throw std::runtime_error(error);
	}

	struct sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(listening_port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sfd, (const sockaddr*)&addr, sizeof(addr)) == -1) {
		close(sfd);
		char error[255];
		sprintf(error, "bind() failed: %s", strerror(errno));
		throw std::runtime_error(error);
	}

	if (listen(sfd, SOMAXCONN) == -1) {
		close(sfd);
		char error[255];
		sprintf(error, "listen() failed: %s", strerror(errno));
		throw std::runtime_error(error);
	}

	if (fcntl(sfd, F_SETFL, O_NONBLOCK) == -1) {
		close(sfd);
		char error[255];
		sprintf(error, "fcntl() failed: %s", strerror(errno));
		throw std::runtime_error(error);
	}

	const int enable = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
		close(sfd);
		char error[255];
		sprintf(error, "setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
		throw std::runtime_error(error);
	}

	int efd = epoll_create1(0);
	if (efd == -1) {
		close(sfd);
		char error[255];
		sprintf(error, "epoll_create1() failed: %s", strerror(errno));
		throw std::runtime_error(error);
	}

	epoll_event ev;
	ev.data.fd = sfd;
	ev.events = EPOLLERR | EPOLLIN;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &ev) == -1) {
		close(sfd);
		close(efd);
		char error[255];
		sprintf(error, "epoll_ctl() failed: %s", strerror(errno));
		throw std::runtime_error(error);
	}

	int pipes[2];
	if (pipe(pipes) == -1) {
		close(sfd);
		close(efd);
		char error[255];
		sprintf(error, "pipe() failed: %s", strerror(errno));
		throw std::runtime_error(error);
	}

	ev.data.fd = pipes[0];
	ev.events = EPOLLIN;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, pipes[0], &ev) == -1) {
		close(sfd);
		close(efd);
		close(pipes[0]);
		close(pipes[1]);
		char error[255];
		sprintf(error, "epoll_ctl(pipe) failed: %s", strerror(errno));
		throw std::runtime_error(error);
	}

	_wait_fd = pipes[0];
	_notifiy_fd = pipes[1];
	_running = true;
	_thr = std::thread(&ipc_server::event_loop, this, efd, sfd);

	log_info(MODULE, "ipc_server listening on port %u", listening_port);
}

void ipc_server::stop() {
	std::unique_lock<std::mutex> lock(_mtx);
	if (_running) {
		_running = false;
		if (_thr.joinable()) {
			notify_one();
			lock.unlock();
			_thr.join();
		}
		close(_notifiy_fd);
		close(_wait_fd);
		_notifiy_fd = -1;
		_wait_fd = -1;

		log_info(MODULE, "ipc_server stopped");
	}
}

void ipc_server::notify_one() {
	char byte;
	write(_notifiy_fd, &byte, 1);
}

void ipc_server::notify_reset() {
	int bytes_available;
	if(ioctl(_wait_fd, FIONREAD, &bytes_available) == 0 && bytes_available > 0) {
		char* dummy = new char[bytes_available];
		read(_wait_fd, dummy, bytes_available);
		delete[] dummy;
	}
}

void ipc_server::event_loop(int efd, int sfd) {

	const int max_events = 20;
	epoll_event events[max_events];
	std::unordered_map<int, ipc_client> clients;
	std::unordered_map<int, std::vector<uint8_t>> session_keys;
	
	while (true) {

		std::unique_lock<std::mutex> lock(_mtx);
		if (!_running) {
			break;
		}

		for (auto& cmd : _commands) {
			if (cmd.second.empty()) {
				log_info(MODULE, "[event_loop] delete key for conn = %d", cmd.first);

				session_keys.erase(cmd.first);
			}
			else {
				log_info(MODULE, "[event_loop] set key for conn = %d", cmd.first);

				for (auto& cl : clients) {
					cl.second.append(cmd.second);
				}
				session_keys.insert(std::move(cmd));
			}
		}
		_commands.clear();
		lock.unlock();

		for (auto it = clients.begin(); it != clients.end();) {
			if (it->second.tick()) {
				++it;
			}
			else {
				it = clients.erase(it);
			}
		}

		int num_of_events = epoll_wait(efd, events, max_events, 100);
		if (num_of_events == -1) {
			log_error(MODULE, "[event_loop] epoll_wait() failed: %s", strerror(errno));
			break;
		}

		for (int i = 0; i < num_of_events; ++i) {
			int fd = events[i].data.fd;
			int ev = events[i].events;

			if (ev & EPOLLERR) {
				if (fd == sfd) {
					log_error(MODULE, "[event_loop] listening socket error");
					break;
				}
				else {
					log_error(MODULE, "[event_loop] client socket error: fd = %d", fd);
					clients.erase(fd);
					continue;
				}
			}

			if (ev & EPOLLIN) {
				if (fd == sfd) {
					sockaddr_in addr;
					socklen_t len = sizeof(addr);
					int cfd = accept(sfd, (sockaddr*)&addr, &len);
					if (cfd == -1) {
						log_error(MODULE, "[event_loop] accept() failed: %s", strerror(errno));
					}
					else {
						ipc_client client(efd, cfd);
						for (auto& pair : session_keys) {
							client.append(pair.second);
						}
						clients.insert(std::make_pair(cfd, std::move(client)));
						log_info(MODULE, "[event_loop] new connection from %s:%u, fd = %d, scheduled %u keys", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), cfd, session_keys.size());
					}
				}
				else if (fd == _notifiy_fd) {
					notify_reset();
				}
				else {
					auto it = clients.find(fd);
					if (it == clients.end()) {
						log_error(MODULE, "[event_loop] epollin: ipc_client (fd = %d) not found", fd);
						epoll_ctl(efd, EPOLL_CTL_DEL, fd, nullptr);
						shutdown(fd, SHUT_RDWR);
						close(fd);
						continue;
					}

					if (!it->second.read()) {
						clients.erase(it);
					}
				}
			}

			if (ev & EPOLLOUT) {

				auto it = clients.find(fd);
				if (it == clients.end()) {
					log_error(MODULE, "[event_loop] epollout: ipc_client (fd = %d) not found", fd);
					epoll_ctl(efd, EPOLL_CTL_DEL, fd, nullptr);
					shutdown(fd, SHUT_RDWR);
					close(fd);
					continue;
				}

				if (!it->second.write()) {
					clients.erase(it);
				}
			}

		}

	}

	clients.clear();
	shutdown(sfd, SHUT_RDWR);
	if (close(sfd) == -1) {
		log_error(MODULE, "close(sfd) failed: %s", strerror(errno));
	}
	if (close(efd) == -1) {
		log_error(MODULE, "close(efd) failed: %s", strerror(errno));
	}
	log_info(MODULE, "event_loop stopped");
}

void ipc_server::set_session_key(
	int conn_id,
	const uint8_t* device_id,
	uint32_t device_id_len,
	const uint8_t* sess_key,
	uint32_t sess_key_len
) {
	std::vector<uint8_t> buffer(16 + device_id_len + sess_key_len);
	*reinterpret_cast<uint32_t*>(buffer.data()) = 0xFFFFFFFF; 				// sync code
	*reinterpret_cast<uint32_t*>(buffer.data() + 4) = htonl(0x00000001); 				// data type: 0x01 - session key
	*reinterpret_cast<uint32_t*>(buffer.data() + 8) = htonl(device_id_len);
	*reinterpret_cast<uint32_t*>(buffer.data() + 12) = htonl(sess_key_len);
	std::memcpy(buffer.data() + 16, device_id, device_id_len);
	std::memcpy(buffer.data() + 16 + device_id_len, sess_key, sess_key_len);

	std::lock_guard<std::mutex> lock(_mtx);
	_commands.insert(std::make_pair(conn_id, std::move(buffer)));
	notify_one();
}

void ipc_server::remove_session_key(int conn_id) {
	std::lock_guard<std::mutex> lock(_mtx);
	_commands.insert(std::make_pair(conn_id, std::vector<uint8_t>()));
}

