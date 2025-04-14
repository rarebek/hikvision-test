#pragma once

#include <thread>
#include <mutex>
#include <map>
#include <string>
#include <vector>
#include <functional>
#include "http_request.hpp"
#include "http_response.hpp"

class http_server {
public:
	using request_handler_cb = std::function<void(const http_request&, http_response&)>;
	void start(request_handler_cb h);
	void stop();

private:
	std::thread _thr;
	std::mutex _mtx;

	int _sfd = -1;
	int _efd = -1;
	request_handler_cb _handle_request;

	void prepare_socket();
	void cleanup();
	void event_loop();
	void accept_client();
	void cleanup_client(int cfd);
	void read_http_request(int cfd);
	void write_http_response(int cfd);

	struct http_client {
		std::vector<char> input_buffer;
		std::vector<char> output_buffer;
		std::size_t out_pos = -1;
	};

	std::map<int, http_client> _clients;
};