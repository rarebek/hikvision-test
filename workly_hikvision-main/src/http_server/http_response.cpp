#include "http_response.hpp"
#include <sstream>
#include <algorithm>

void http_response::set_status(int code, std::string text) {
	_status_code = code;
	_status_text = text;
}

void http_response::set_header(std::string key, std::string value) {
	std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
		return std::tolower(ch);
	});
	_headers[key] = value;
}

void http_response::set_body(std::vector<char>&& body) {
	_body = body;
}

void http_response::write(const std::vector<char>& chunk) {
	_body.insert(_body.end(), chunk.begin(), chunk.end());
}

void http_response::write(const char* data, std::size_t size) {
	_body.insert(_body.end(), data, data + size);
}

void http_response::end() {
	std::string head = "HTTP/1.1 " + std::to_string(_status_code) + " " + _status_text + "\r\n";

	if (_body.size()) {
		_headers["content-length"] = std::to_string(_body.size());
	}
	
	for (auto& h : _headers) {
		head += h.first + ": " + h.second + "\r\n";
	}
	head += "\r\n";
	_data.assign(head.begin(), head.end());
	_data.insert(_data.end(), _body.begin(), _body.end());
}