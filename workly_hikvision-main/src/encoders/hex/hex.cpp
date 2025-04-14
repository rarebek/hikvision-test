#include "hex.hpp"

std::string hex_encode(const std::vector<char>& data) {
	const char lut[16] = {
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'a', 'b',
		'c', 'd', 'e', 'f'
	};

	std::string s;
	for( auto& byte : data ) {
		unsigned char c = (unsigned char) byte;
		s += lut[c >> 4];
		s += lut[c & 0x0F];
	}
	return s;
}

std::string hex_encode(const std::vector<unsigned char>& data) {
	const char lut[16] = {
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'a', 'b',
		'c', 'd', 'e', 'f'
	};

	std::string s;
	for( auto& byte : data ) {
		s += lut[byte >> 4];
		s += lut[byte & 0x0F];
	}
	return s;
}

std::string hex_encode(const char* data, std::size_t len) {

	const char lut[16] = {
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'a', 'b',
		'c', 'd', 'e', 'f'
	};
	const unsigned char* udata = (const unsigned char*)data;

	std::string s;
	for (std::size_t i = 0; i < len; ++i) {
		s += lut[udata[i] >> 4];
		s += lut[udata[i] & 0x0F];
	}
	return s;

}

std::string hex_encode(const unsigned char* data, std::size_t len) {

	const char lut[16] = {
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'a', 'b',
		'c', 'd', 'e', 'f'
	};

	std::string s;
	for (std::size_t i = 0; i < len; ++i) {
		s += lut[data[i] >> 4];
		s += lut[data[i] & 0x0F];
	}
	return s;
}

std::vector<char> hex_decode(const std::string& hex) {
	char lut[128];
	memset(lut, 0, 128);
	for( int i = 'A'; i <= 'F'; ++i ) {
		lut[i] = i - 'A' + 0x0A;
	}
	for( int i = 'a'; i <= 'f'; ++i ) {
		lut[i] = i - 'a' + 0x0A;
	}
	for( int i = '0'; i <= '9'; ++i ) {
		lut[i] = i - '0';
	}
	std::vector<char> data;
	for( int i = 0; i + 1 < hex.size(); i += 2 ) {
		int high = lut[hex[i]];
		int low = lut[hex[i + 1]];
		data.push_back(high << 4 | low);
	}

	return data;
}