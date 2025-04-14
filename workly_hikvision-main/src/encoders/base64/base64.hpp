#pragma once

#include <string>
#include <vector>
#include <cstring>

std::string base64_encode(const char* data, int size, bool url_encode);
std::vector<char> base64_decode(const std::string& text);