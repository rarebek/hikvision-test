#pragma once

#include <string>
#include <vector>
#include <map>

class http_request {
public:
    http_request(const std::vector<char>& raw_request);
    
    const std::string& method() const { return _method;}
    const std::string& path() const { return _path;}
    const std::string& protocol() const { return _protocol;}
    const std::map<std::string, std::string>& headers() const { return _headers;}
    const std::vector<char>& body() const { return _body;}
    bool header(const std::string& key, std::string& value);
    
private:
    std::string _method;
    std::string _path;
    std::string _protocol;
    std::map<std::string, std::string> _headers;
    std::vector<char> _body;
    
private:
    
};

