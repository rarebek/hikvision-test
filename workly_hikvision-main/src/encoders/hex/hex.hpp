#pragma once

#include <string>
#include <vector>
#include <cstring>

std::string hex_encode(const std::vector<char>& data);
std::string hex_encode(const std::vector<unsigned char>& data);
std::string hex_encode(const char* data, std::size_t len);
std::string hex_encode(const unsigned char* data, std::size_t len);
std::vector<char> hex_decode(const std::string& hex);