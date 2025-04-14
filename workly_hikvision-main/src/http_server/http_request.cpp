#include "http_request.hpp"
#include <algorithm>
#include <system_error>

std::vector<std::string> split(const std::string& s, const std::string& delim) {
    
    std::vector<std::string> result;
    std::size_t pos = 0;
    while (true) {
        
        auto pos2 = s.find(delim, pos);
        if (pos2 == std::string::npos) {
            if (pos < s.length()) {
                result.push_back(s.substr(pos));
            }
            break;
        }
        else {
            result.push_back(s.substr(pos, pos2 - pos));
            pos = pos2 + delim.size();
        }
        
    }
    
    return result;
}

std::map<std::string, std::string> parse_headers(const std::vector<std::string>& raw_headers) {
    
    std::map<std::string, std::string> headers;
    for (auto& rh : raw_headers) {
        
        auto pos = rh.find(":");
        if (pos == std::string::npos) {
            continue;
        }
        
        std::string key = rh.substr(0, pos);
        std::string value = rh.substr(pos + 1);
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
            return std::tolower(ch);
        });
        headers.insert(std::make_pair(key, value));
    }
    
    return headers;
}

void parse_status_line(const std::string& line, std::string& method, std::string& path, std::string& protocol) {
    auto lines = split(line, " ");
    if (lines.size() != 3) {
        throw std::runtime_error("Invalid status line: " + line);
    }
    method = lines[0];
    path = lines[1];
    protocol = lines[2];
    
    std::transform(method.begin(), method.end(), method.begin(), [](unsigned char ch) {
        return std::tolower(ch);
    });
    std::transform(path.begin(), path.end(), path.begin(), [](unsigned char ch) {
        return std::tolower(ch);
    });
    std::transform(protocol.begin(), protocol.end(), protocol.begin(), [](unsigned char ch) {
        return std::tolower(ch);
    });
}

http_request::http_request(const std::vector<char>& raw_request) {
    
    static std::string end_of_head = "\r\n\r\n";
    auto it = std::search(raw_request.begin(), raw_request.end(), end_of_head.begin(), end_of_head.end());
    if (it == raw_request.end()) {
        throw std::runtime_error("invalid http request. could't find end of http head");
    }
    
    std::string head(raw_request.begin(), it);
    
    auto status_end = head.find("\r\n");
    if (status_end == std::string::npos) {
        throw std::runtime_error("invalid http request. couldn't find end of status line");
    }
    
    std::string method, path, protocol;
    parse_status_line(head.substr(0, status_end), method, path, protocol);
    
    auto raw_headers = split(head.substr(status_end + 2), "\r\n");
    auto headers = parse_headers(raw_headers);
    _method = method;
    _path = path;
    _protocol = protocol;
    _headers = headers;
    
    _body.assign(it + 4, raw_request.end());
}

bool http_request::header(const std::string& key, std::string& value) {
    if (_headers.count(key)) {
        value = _headers[key];
        return true;
    }
    return false;
}



