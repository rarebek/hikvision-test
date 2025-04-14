#pragma once

#include "encoders/nlohmann/json.hpp"
#include "encoders/pugixml/pugixml.hpp"
#include "../../libisup/include/HCISUPCMS.h"
#include "../../libisup/include/HCISUPAlarm.h"
#include "../../libisup/include/HCISUPSS.h"
#include <string>

namespace request {

	void check_response(nlohmann::json& data);
	void check_response(pugi::xml_document& data);

	nlohmann::json get(long conn_id, std::string url, DWORD timeout = 30000);
	nlohmann::json post(long conn_id, std::string url, const nlohmann::json& data, DWORD timeout = 30000);
	nlohmann::json put(long conn_id, std::string url, const nlohmann::json& data, DWORD timeout = 30000);

	pugi::xml_document post(long conn_id, std::string url, pugi::xml_document& data, DWORD timeout = 30000);
	pugi::xml_document put(long conn_id, std::string url, pugi::xml_document& data, DWORD timeout = 30000);
	
}