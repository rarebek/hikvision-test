#pragma once

#include <vector>
#include <string>
#include <map>

class http_response {
public:

	void set_status(int code, std::string text);
	void set_header(std::string key, std::string value);
	void set_body(std::vector<char>&& body);
	void write(const std::vector<char>& chunk);
	void write(const char* data, std::size_t size);
	void end();
	
	const std::vector<char>& data() {
		return _data;
	}

private:
	int _status_code;
	std::string _status_text;
	std::map<std::string, std::string> _headers;
	std::vector<char> _body;
	std::vector<char> _data;
};