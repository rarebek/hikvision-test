#include "request.hpp"
#include <system_error>
#include <sstream>
#include <iostream>

namespace request {

	enum class method { get, post, put };

	std::vector<char> fetch(long conn_id, method method, std::string url, std::string&& data, DWORD timeout) {

		NET_EHOME_PTXML_PARAM param = {0};
		param.pRequestUrl = (void*)url.data();
		param.dwRequestUrlLen = url.size();
		param.dwRecvTimeOut = timeout;

		if (!data.empty()) {
			param.pInBuffer = data.data();
			param.dwInSize = data.size();
		}

		std::vector<char> buffer(512 * 1024);
		memset(buffer.data(), 0, buffer.size());
		param.pOutBuffer = (void*)buffer.data();
		param.dwOutSize = buffer.size();

		bool success;
		std::string method_name;
		switch (method) {
		case method::get:
			success = NET_ECMS_GetPTXMLConfig(conn_id, &param);
			method_name = "GET";
			break;
		case method::post:
			success = NET_ECMS_PostPTXMLConfig(conn_id, &param);
			method_name = "POST";
			break;
		case method::put:
			success = NET_ECMS_PutPTXMLConfig(conn_id, &param);
			method_name = "PUT";
			break;
		}

		if (!success) {
			DWORD error_code = NET_ECMS_GetLastError();
			std::string error_msg = "unknown";
			switch (error_code) {
			case NET_DVR_NETWORK_RECV_TIMEOUT:
				error_msg = "NET_DVR_NETWORK_RECV_TIMEOUT";
				break;
			case NET_DVR_NETWORK_SEND_ERROR:
				error_msg = "NET_DVR_NETWORK_SEND_ERROR";
				break;
			case NET_DVR_NETWORK_FAIL_CONNECT:
				error_msg = "NET_DVR_NETWORK_FAIL_CONNECT";
				break;
			case NET_DVR_NETWORK_RECV_ERROR:
				error_msg = "NET_DVR_NETWORK_RECV_ERROR";
				break;
			}

			std::stringstream ss;
			ss << "<===============REQUEST===============>" << std::endl;
			ss << method_name << " " << url << std::endl;
			ss << data << std::endl << std::endl;
			ss << "<===============ERROR===============>" << std::endl;
			ss << "code = " << error_code << ", message = " << error_msg << std::endl << std::endl;

			throw std::runtime_error(error_msg.c_str());
		}

		return buffer;
	}


	nlohmann::json get(long conn_id, std::string url, DWORD timeout) {
		auto raw_response = fetch(conn_id, method::get, url, "", timeout);
		auto response = nlohmann::json::parse(raw_response);
		response["_request_method_"] = "GET";
		response["_request_url_"] = url;
		response["_request_body_"] = {};
		return response;
	}
	
	nlohmann::json post(long conn_id, std::string url, const nlohmann::json& data, DWORD timeout) {
		auto raw_response = fetch(conn_id, method::post, url, data.dump(), timeout);
		auto response = nlohmann::json::parse(raw_response);
		response["_request_method_"] = "POST";
		response["_request_url_"] = url;
		response["_request_body_"] = data;
		return response;
	}

	nlohmann::json put(long conn_id, std::string url, const nlohmann::json& data, DWORD timeout) {
		auto raw_response = fetch(conn_id, method::put, url, data.dump(), timeout);
		auto response = nlohmann::json::parse(raw_response);
		response["_request_method_"] = "PUT";
		response["_request_url_"] = url;
		response["_request_body_"] = data;
		return response;
	}

	pugi::xml_document post(long conn_id, std::string url, pugi::xml_document& data, DWORD timeout) {
		std::stringstream ss; data.save(ss);
		auto raw_response = fetch(conn_id, method::post, url, ss.str(), timeout);

		pugi::xml_document doc;
		doc.load_buffer(raw_response.data(), raw_response.size());
		doc.append_child("_request_method_").append_child(pugi::node_pcdata).set_value("POST");
		doc.append_child("_request_url_").append_child(pugi::node_pcdata).set_value(url.c_str());
		doc.append_child("_request_body_").append_child(pugi::node_pcdata).set_value(ss.str().c_str());
		return doc;
	}
	
	pugi::xml_document put(long conn_id, std::string url, pugi::xml_document& data, DWORD timeout) {
		std::stringstream ss;
		data.save(ss);
		auto raw_response = fetch(conn_id, method::put, url, ss.str(), timeout);
		
		pugi::xml_document doc;
		doc.load_buffer(raw_response.data(), raw_response.size());
		doc.append_child("_request_method_").append_child(pugi::node_pcdata).set_value("POST");
		doc.append_child("_request_url_").append_child(pugi::node_pcdata).set_value(url.c_str());
		doc.append_child("_request_body_").append_child(pugi::node_pcdata).set_value(ss.str().c_str());
		return doc;
	}

	void check_response(nlohmann::json& response) {
		std::string request_method = response["_request_method_"];
		std::string request_url = response["_request_url_"];
		std::string request_body = response["_request_body_"].dump(2);
		response.erase("_request_method_");
		response.erase("_request_url_");
		response.erase("_request_body_");

		int status_code = 0;
		if (response.contains("statusCode")) {
			status_code = response["statusCode"];
		}
		if (status_code != 1) {
			std::stringstream ss;
			ss << "<===============REQUEST===============>" << std::endl;
			ss << request_method << " " << request_url << std::endl;
			ss << request_body << std::endl << std::endl;
			ss << "<===============RESPONSE===============>" << std::endl;
			ss << response.dump(2) << std::endl << std::endl;
			throw std::runtime_error(ss.str());
		}
	}
	
	void check_response(pugi::xml_document& doc) {
		std::string request_method = doc.child("_request_method_").child_value();
		std::string request_url = doc.child("_request_url_").child_value();
		std::string request_body = doc.child("_request_body_").child_value();
		doc.remove_child("_request_method_");
		doc.remove_child("_request_url_");
		doc.remove_child("_request_body_");
		int status_code = std::atol(doc.child("ResponseStatus").child("statusCode").child_value());
		if (status_code != 1) {
			std::stringstream ss;
			ss << "<===============REQUEST===============>" << std::endl;
			ss << request_method << " " << request_url << std::endl;
			ss << request_body << std::endl << std::endl;
			ss << "<===============RESPONSE===============>" << std::endl;
			doc.save(ss);
			ss << std::endl << std::endl;
			throw std::runtime_error(ss.str());
		}
	}
}