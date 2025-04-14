#include "http_server.hpp"
#include "../config/config.hpp"
#include "../logger/logger.hpp"

#include <algorithm>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>


#define HTTP_MODULE "HTTP"

void http_server::start(request_handler_cb h) {
	std::lock_guard<std::mutex> lock(_mtx);
	prepare_socket();
	_handle_request = h;
	_thr = std::thread(&http_server::event_loop, this);

	log_info(HTTP_MODULE, "http_server listening on port %u", config::http_server_port());
}

void http_server::stop() {

	std::unique_lock<std::mutex> lock(_mtx);
	cleanup();
	lock.unlock();

	if (_thr.joinable()) {
		_thr.join();
	}
	log_info(HTTP_MODULE, "http_server stopped");
}

void http_server::prepare_socket() {

	int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sfd == -1) {
		throw std::runtime_error("[http] socket() failed: error = " + std::to_string(errno));
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(config::http_server_port());
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sfd, (sockaddr*)&addr, sizeof(addr)) == -1) {
		close(sfd);
		throw std::runtime_error("[http] bind() failed: error = " + std::to_string(errno));
	}

	if (listen(sfd, SOMAXCONN) == -1) {
		close(sfd);
		throw std::runtime_error("[http] listen() failed: error = " + std::to_string(errno));
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
		throw std::runtime_error("[http] epoll_create1() failed: error = " + std::to_string(errno));
	}

	epoll_event e;
	e.events = EPOLLIN | EPOLLERR;
	e.data.fd = sfd;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &e) == -1) {
		close(sfd);
		close(efd);
		throw std::runtime_error("[http] epoll_ctl() failed: error = " + std::to_string(errno));
	}

	_sfd = sfd;
	_efd = efd;
}

void http_server::cleanup() {
	if (_sfd != -1) {
		shutdown(_sfd, SHUT_RDWR);
		close(_sfd);
		_sfd = -1;
	}
	
	if (_efd) {
		close(_efd);
		_efd = -1;
	}
}

void http_server::event_loop() {

	const int max_events = 20;
	epoll_event events[max_events];

	while (true) {

		int num_of_events = epoll_wait(_efd, events, max_events, 100);
		if (num_of_events == -1) {
			int error = errno;
			log_error(HTTP_MODULE, "epoll_wait(): %s", strerror(error));
			break;
		}

		for (int i = 0; i < num_of_events; ++i) {
			int fd = events[i].data.fd;
			if (events[i].events & EPOLLIN) {
				if (fd == _sfd) {
					accept_client();
				}
				else {
					read_http_request(events[i].data.fd);
				}
			}
			else if (events[i].events & EPOLLERR) {
				if (fd == _sfd) {
					log_error(HTTP_MODULE, "main socket error");
					cleanup();
					return;
				}
				else {
					log_error(HTTP_MODULE, "client socket error");
					cleanup_client(fd);
				}
			}
			else if (events[i].events & EPOLLOUT) {
				write_http_response(fd);
			}
		}

	}

}

void http_server::accept_client() {

	constexpr int buf_size = 200;
	unsigned char addr_buf[buf_size];
	std::memset(addr_buf, 0, buf_size);
	
	auto addr = reinterpret_cast<sockaddr*>(addr_buf);
	socklen_t len = buf_size;

	int cfd = accept4(_sfd, addr, &len, SOCK_NONBLOCK);
	if (cfd == -1) {
		int error = errno;
		log_error(HTTP_MODULE, "accept4(): errno = %d, msg = %s. clients count = %u", error, strerror(error), _clients.size());
		if (error == EMFILE) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}
	else {
		std::string ip;
		uint16_t port;
		if (addr->sa_family == AF_INET && len < buf_size) {
			auto ipv4 = reinterpret_cast<sockaddr_in*>(addr);
			char buf[20];
			inet_ntop(AF_INET, &ipv4->sin_addr, buf, sizeof(buf));
			ip = buf;
			port = ntohs(ipv4->sin_port);
		}
		else if (addr->sa_family == AF_INET && len < buf_size) {
			auto ipv6 = reinterpret_cast<sockaddr_in6*>(addr);
			char buf[100];
			inet_ntop(AF_INET6, &ipv6->sin6_addr, buf, sizeof(buf));
			ip = buf;
			port = ntohs(ipv6->sin6_port);
		}

		epoll_event e;
		e.events = EPOLLIN | EPOLLERR;
		e.data.fd = cfd;
		if (epoll_ctl(_efd, EPOLL_CTL_ADD, cfd, &e) == -1) {
			close(cfd);
			int error = errno;
			log_error(HTTP_MODULE, "epoll_ctl(%s:%u): errno = %d, msg = %s", ip.c_str(), port, error, strerror(error));
		}
		else {
			log_info(HTTP_MODULE, "new client (fd=%d) from %s:%u. clients_count = %u", cfd, ip.c_str(), port, _clients.size());
		}
	}
}

void http_server::cleanup_client(int cfd) {
	_clients.erase(cfd);
	epoll_ctl(_efd, EPOLL_CTL_DEL, cfd, NULL);
	shutdown(cfd, SHUT_RDWR);
	close(cfd);
}

void http_server::read_http_request(int cfd) {

	if (_clients.count(cfd) == 0) {
		_clients.insert(std::make_pair(cfd, http_client()));
	}
	auto& client = _clients.at(cfd);

	constexpr int buf_size = 256 * 1024;
	char rcv_buf[buf_size];
	while (true) {

		int bytes_read = read(cfd, rcv_buf, buf_size);
		if (bytes_read == -1) {
			int error = errno;
			if (error != EAGAIN) {
				log_error(HTTP_MODULE, "read(fd = %d): error = %d, msg = %s", cfd, error, strerror(error));
				cleanup_client(cfd);
			}
			break;
		}
		else if (bytes_read == 0) {
			cleanup_client(cfd);
			break;
		}
		else {
			client.input_buffer.insert(
				client.input_buffer.end(),
				rcv_buf, rcv_buf + bytes_read
			);

			// check for http header
			static std::string end_of_head = "\r\n\r\n";
			auto it = std::search(client.input_buffer.begin(), client.input_buffer.end(), end_of_head.begin(), end_of_head.end());
			if (it == client.input_buffer.end()) {
				continue;
			}

			try {
				http_request request(client.input_buffer);
				std::string content_length;
				if (request.header("content-length", content_length)) {
					long long length = std::atoll(content_length.c_str());
					if (length != request.body().size()) {
						continue; // read full http body
					}
				}

				// stop reading and start sending
				http_response response;
				_handle_request(request, response);
				client.input_buffer.clear();
				client.output_buffer = response.data();

				epoll_event e;
				e.events = EPOLLOUT | EPOLLERR;
				e.data.fd = cfd;
				epoll_ctl(_efd, EPOLL_CTL_MOD, cfd, &e);
			}
			catch (const std::exception& e) {
				log_error(HTTP_MODULE, "read_http_request(fd = %d): error = %s", cfd, e.what());
				cleanup_client(cfd);
			}

			
		}

	}

	
}

void http_server::write_http_response(int cfd) {
	auto& client = _clients.at(cfd);

	if (client.out_pos == -1) {
		client.out_pos = 0;
	}

	while (true) {

		int bytes_written = write(
			cfd,
			client.output_buffer.data() + client.out_pos,
			client.output_buffer.size() - client.out_pos
		);
		
		if (bytes_written == -1) {
			int error = errno;
			if (error != EAGAIN) {
				log_error(HTTP_MODULE, "write(cfd = %d) failed. error = %d, msg = %s", cfd, error, strerror(error));
				cleanup_client(cfd);
			}
			break;
		}
		else if (bytes_written > 0) {
			client.out_pos += bytes_written;
			if (client.out_pos == client.output_buffer.size()) {
				// end of response
				cleanup_client(cfd);
				break;
			}
		}
		else {
			log_error(HTTP_MODULE, "write(cfd = %d) written 0 bytes", cfd);
			cleanup_client(cfd);
			break;
		}
	}

}
