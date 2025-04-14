#include "device.hpp"
#include "request.hpp"
#include "logger/logger.hpp"
#include <string>
#include <set>
#include <sstream>

device::device(long conn_id, std::string device_sn, std::string device_id, std::string device_ip, unsigned short device_port) {
	_conn_id = conn_id;
	_device_sn = device_sn;
	_device_id = device_id;
	_device_ip = device_ip;
	_device_port = device_port;
	_connected = true;
}

device::~device() {}


void device::add_user(std::string user_id, std::string name, std::string password, bool is_admin) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	nlohmann::json data = {
		{"UserInfo", {
			{"employeeNo", user_id},
			{"name", name},
			{"userType", "normal"},
			{"gender", "male"},
			{"Valid", {
				{"enable", true},
				{"beginTime", "2021-01-10T00:00:00"},
				{"endTime", "2030-01-10T00:00:00"},
				{"timeType", "local"},
			}},
			{"password", password},
			{"localUIRight", is_admin},
			{"userVerifyMode", ""},
			{"maxOpenDoorTime", 0},
			{"doorRight", "1"},
			{"RightPlan", {
				{
					{"doorNo", 1},
					{"planTemplateNo", "1"}
				}
			}}
		}}
	};

	if (is_user_exist(user_id)) {
		auto response = request::put(_conn_id, "/ISAPI/AccessControl/UserInfo/Modify?format=json", data);
		request::check_response(response);
	}
	else {
		auto response = request::post(_conn_id, "/ISAPI/AccessControl/UserInfo/Record?format=json", data);
		request::check_response(response);
		
		// old user's face data may still be there
		del_facedata(user_id);
	}
}

void device::del_user(std::string user_id) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	auto response = request::put(_conn_id, "/ISAPI/AccessControl/UserInfo/Delete?format=json", nlohmann::json({
		{"UserInfoDelCond", {
			{"EmployeeNoList", {
				{{"employeeNo", user_id}}
			}}
		}}
	}));
	request::check_response(response);
}

void device::get_users(std::list<device_user>& list) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	bool more = true;
	while (more) {
		nlohmann::json response = request::post(_conn_id, "/ISAPI/AccessControl/UserInfo/Search?format=json", nlohmann::json({
			{"UserInfoSearchCond", {
				{"searchID", "1"},
				{"searchResultPosition", list.size()},
				{"maxResults", 100}
			}}
		}));
		nlohmann::json search = response.at("UserInfoSearch");
		more = search.at("responseStatusStrg") == "MORE";
		int total = search.at("totalMatches");
		int count = search.at("numOfMatches");
		nlohmann::json users = search.at("UserInfo");
		for (int i = 0; i < count; ++i) {
			nlohmann::json user = users.at(i);
			device_user u;
			u.user_id = user.at("employeeNo");
			u.name = user.at("name");
			u.password = user.at("password");
			list.push_back(u);
		}
	}
	
}

void device::clear_users() {
	
	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	auto response = request::put(_conn_id, "/ISAPI/AccessControl/UserInfo/Delete?format=json", nlohmann::json({
		{"UserInfoDelCond", {
			{"EmployeeNoList", {}}
		}}
	}));
	request::check_response(response);
}

bool device::is_user_exist(std::string user_id) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	nlohmann::json response = request::post(_conn_id, "/ISAPI/AccessControl/UserInfo/Search?format=json", nlohmann::json({
		{"UserInfoSearchCond", {
			{"searchID", "1"},
			{"searchResultPosition", 0},
			{"maxResults", 20},
			{"EmployeeNoList", {
				{{"employeeNo", user_id}}
			}}
		}}
	}));
	return response.at("UserInfoSearch").at("responseStatusStrg") != "NO MATCH";
}


void device::add_card(const std::string& user_id, const std::string& card_no) {
	
	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	auto response = request::put(_conn_id, "/ISAPI/AccessControl/CardInfo/SetUp?format=json", nlohmann::json({
		{"CardInfo", {
			{"employeeNo", user_id},
			{"cardNo", card_no},
			{"cardType", "normalCard"},
		}}
	}));
	request::check_response(response);
}

void device::del_card(const std::string& card_no) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	auto response = request::put(_conn_id, "/ISAPI/AccessControl/CardInfo/Delete?format=json", nlohmann::json({
		{"CardInfoDelCond", {
			{"CardNoList", {
				{{"cardNo", card_no}}
			}}
		}}
	}));
	request::check_response(response);
}

void device::del_user_cards(const std::string& user_id) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	auto response = request::put(_conn_id, "/ISAPI/AccessControl/CardInfo/Delete?format=json", nlohmann::json({
		{"CardInfoDelCond", {
			{"EmployeeNoList", {
				{{"employeeNo", user_id}}
			}}
		}}
	}));
	request::check_response(response);
}

void device::get_cards(const std::string& user_id, std::list<std::string>& list) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	auto response = request::post(_conn_id, "/ISAPI/AccessControl/CardInfo/Search?format=json", nlohmann::json({
		{"CardInfoSearchCond", {
			{"searchID", "1"},
			{"searchResultPosition", 0},
			{"maxResults", 10},
			{"EmployeeNoList", {
				{{"employeeNo", user_id}}
			}}
		}}
	}));

	nlohmann::json info = response.at("CardInfoSearch");
	int count = info.at("numOfMatches");
	if (count == 0) {
		throw std::runtime_error("No cards found");
	}
	nlohmann::json cards = info.at("CardInfo");
	for (int i = 0; i < count; ++i) {
		nlohmann::json card = cards.at(i);
		if (card.at("cardType") == "normalCard") {
			list.push_back(card.at("cardNo"));
		}
	}

}


void device::set_face_url(const std::string& user_id, const std::string& face_url) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	nlohmann::json response = request::put(_conn_id, "/ISAPI/Intelligent/FDLib/FDSetUp?format=json", nlohmann::json({
		{"faceLibType", "blackFD"},
		{"FDID", get_blackfd()},
		{"FPID", user_id},
		{"faceURL", face_url}
	}));
	request::check_response(response);
}

void device::get_face_url(const std::string& user_id, std::string& face_url) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	nlohmann::json response = request::post(_conn_id, "/ISAPI/Intelligent/FDLib/FDSearch?format=json", nlohmann::json({
		{"searchResultPosition", 0},
		{"maxResults", 10},
		{"faceLibType", "blackFD"},
		{"FDID", get_blackfd()},
		{"FPID", user_id}
	}));

	int count = response.at("numOfMatches");
	if (count == 0) {
		throw std::runtime_error("No faces exists");
	}
	face_url = response.at("MatchList").at(0).at("faceURL");
}

void device::del_facedata(const std::string& user_id) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	nlohmann::json response = request::put(_conn_id, "/ISAPI/Intelligent/FDLib/FDSetUp?format=json", nlohmann::json({
		{"faceLibType", "blackFD"},
		{"FDID", get_blackfd()},
		{"FPID", user_id},
		{"deleteFP", true}
	}));
	request::check_response(response);
}

std::string device::get_facelib_id(std::string facelib) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	nlohmann::json response = request::get(_conn_id, "/ISAPI/Intelligent/FDLib/Count?format=json");
	nlohmann::json records = response.at("FDRecordDataInfo");
	for (auto& r : records) {
		if (r.at("faceLibType") == facelib) {
			return r.at("FDID");
		}
	}
	throw std::runtime_error("\"" + facelib + "\" not found");
}

std::string device::get_blackfd() {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	if (_blackfd.empty()) {
		_blackfd = get_facelib_id("blackFD");
	}
	return _blackfd;
}


void device::set_fingerprint(const std::string& user_id, int fp_id, const std::string& b64_fp_data) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	nlohmann::json response = request::post(_conn_id, "/ISAPI/AccessControl/FingerPrint/SetUp?format=json", nlohmann::json({
		{"FingerPrintCfg", {
			{"employeeNo", user_id},
			{"enableCardReader", {1}},
			{"fingerPrintID", fp_id},
			{"fingerType", "normalFP"},
			{"fingerData", b64_fp_data},
		}}
	}));
	if (response.contains("FingerPrintStatus")) {
		nlohmann::json status = response.at("FingerPrintStatus");
		if (status.contains("StatusList") && status.at("StatusList").size() > 0) {
			nlohmann::json item = status.at("StatusList")[0];
			if (item.contains("cardReaderRecvStatus")) {
				int reader_status = item.at("cardReaderRecvStatus");
				switch(reader_status) {
					case 0: throw std::runtime_error("connecting failed");
					case 1: return;
					case 2: throw std::runtime_error("the fingerprint module is offline");
					case 3: throw std::runtime_error("the fingerprint quality is poor, try again");
					case 4: throw std::runtime_error("the memory is full");
					case 5: throw std::runtime_error("the fingerprint already exists");
					case 6: throw std::runtime_error("the fingerprint ID already exists");
					case 7: throw std::runtime_error("invalid fingerprint ID");
					case 8: throw std::runtime_error("this fingerprint module is already configured");
					case 9: throw std::runtime_error("the fingerprint module version is too old to support the employee No.");
				}
				if (item.contains("errorMsg")) {
					throw std::runtime_error(item.at("errorMsg"));
				}
			}
		}
	}
	throw std::runtime_error(response.dump());
}

void device::get_fingerprints(const std::string& user_id, std::list<std::pair<std::string, int>>& fp_list) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}
	
	std::string search_id = std::to_string(std::time(nullptr));

	while (true) {
		nlohmann::json response = request::post(_conn_id, "/ISAPI/AccessControl/FingerPrintUpload?format=json", nlohmann::json({
			{"FingerPrintCond", {
				{"searchID", search_id},
				{"employeeNo", user_id}
			}}
		}));

		log_debug(_device_sn, "get_fingerprint(%s)\n%s", user_id.c_str(), response.dump().c_str());
		
		nlohmann::json FingerPrintInfo = response.at("FingerPrintInfo");
		std::string status = FingerPrintInfo.at("status");

		// "OK"-the fingerprint exists, "NoFP"-the fingerprint does not exist
		if (status == "OK") {
			nlohmann::json FingerPrintList = FingerPrintInfo.at("FingerPrintList");
			for(const auto& item: FingerPrintList) {
				int cardReaderNo = item.at("cardReaderNo");
				int fingerPrintID = item.at("fingerPrintID");
				std::string fingerType = item.at("fingerType");
				std::string fingerData = item.at("fingerData");
				fp_list.push_back(std::make_pair(fingerData, fingerPrintID));
			}
		}
		else {
			log_debug(_device_sn, "get_fingerprint(%s) done. got %u fp data", user_id.c_str(), fp_list.size());
			break;
		}
	}

}

void device::del_fingerprint(const std::string& user_id, int fp_id) {
	
	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	nlohmann::json response = request::post(_conn_id, "/ISAPI/AccessControl/FingerPrint/SetUp?format=json", nlohmann::json({
		{"FingerPrintCfg", {
			{"employeeNo", user_id},
			{"fingerPrintID", fp_id},
			{"fingerType", "normalFP"},
			{"deleteFingerPrint", true},
		}}
	}));

	if (response.contains("FingerPrintStatus")) {
		if(response.at("FingerPrintStatus").at("status") == "success") {
			return;
		}
	}
	throw std::runtime_error(response.dump());
}


void device::get_records(mylib::date start, mylib::date end, std::list<device_event>& list, mylib::date& next_event_date) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	list.clear();
	int position = 0;
	bool has_more_data = true;

	log_debug(_device_sn, "get_records(%s, %s) begin", start.iso_string().c_str(), end.iso_string().c_str());

	while (has_more_data) {
		
		nlohmann::json request;
		request["AcsEventCond"]["searchID"] = std::to_string(std::time(nullptr));
		request["AcsEventCond"]["searchResultPosition"] = position;
		request["AcsEventCond"]["maxResults"] = 20;
		request["AcsEventCond"]["major"] = MajorTypes::MAJOR_EVENT;
		request["AcsEventCond"]["minor"] = 0;
		request["AcsEventCond"]["startTime"] = start.iso_string();
		request["AcsEventCond"]["endTime"] = end.iso_string();

		nlohmann::json response = request::post(_conn_id, "/ISAPI/AccessControl/AcsEvent?format=json", request);
		if (response.contains("statusCode")) {
			request::check_response(response);
		}

		try {
			std::string status = response.at("AcsEvent").at("responseStatusStrg");
			if (status == "NO MATCH") {
				log_debug(_device_sn, "get_records(%s, %s) end: no match", start.iso_string().c_str(), end.iso_string().c_str());
				return;
			}

			int num_of_matches = response.at("AcsEvent").at("numOfMatches");
			nlohmann::json event_list = response.at("AcsEvent").at("InfoList");

			for (int i = 0; i < num_of_matches; ++i) {
				nlohmann::json item = event_list.at(i);

				if (item.contains("employeeNoString")) {
					device_event event;
					event.row_id = item.at("serialNo");
					event.user_id = item.at("employeeNoString");
					event.event_date = item.at("time");
					event.attendance = attendance::ATTENDANCE_UNKNOWN;
					event.unlock_method = unlock_method::UNLOCK_UNKNOWN;

					switch ((int)item.at("minor")) {
						case MinorTypes_MajorEvent::MINOR_LEGAL_CARD_PASS:
						case MinorTypes_MajorEvent::MINOR_CARD_AND_PSW_PASS:
						case MinorTypes_MajorEvent::MINOR_CARD_FINGERPRINT_VERIFY_PASS:
						case MinorTypes_MajorEvent::MINOR_CARD_FINGERPRINT_PASSWD_VERIFY_PASS:
							event.unlock_method = unlock_method::UNLOCK_CARD;
							break;
						case MinorTypes_MajorEvent::MINOR_FINGERPRINT_PASSWD_VERIFY_PASS:
						case MinorTypes_MajorEvent::MINOR_EMPLOYEENO_AND_FP_VERIFY_PASS:
						case MinorTypes_MajorEvent::MINOR_EMPLOYEENO_AND_FP_AND_PW_VERIFY_PASS:
						case MinorTypes_MajorEvent::MINOR_FINGERPRINT_COMPARE_PASS:
							event.unlock_method = unlock_method::UNLOCK_FINGERPRINT;
							break;
						case MinorTypes_MajorEvent::MINOR_FACE_VERIFY_PASS:
						case MinorTypes_MajorEvent::MINOR_EMPLOYEENO_AND_FACE_VERIFY_PASS:
							event.unlock_method = unlock_method::UNLOCK_FACE;
							break;
						case MinorTypes_MajorEvent::MINOR_EMPLOYEENO_AND_PW_PASS:
						case MinorTypes_MajorEvent::MINOR_PASSWD_VERIFY_PASS:
							event.unlock_method = unlock_method::UNLOCK_PASSWORD;
							break;
					}

					if (item.contains("attendanceStatus")) {
						std::string attendance_status = item.at("attendanceStatus");
						static std::set<std::string> check_in = { "checkIn", "breakIn", "overtimeIn" };
						static std::set<std::string> check_out = {"checkOut", "breakOut", "overtimeOut"};
						if (check_in.count(attendance_status) > 0) {
							event.attendance = attendance::ATTENDANCE_CHECKIN;
						}
						else if (check_out.count(attendance_status) > 0) {
							event.attendance = attendance::ATTENDANCE_CHECKOUT;
						}
						else {
							log_error(_device_sn, "Unknown attendance: %s", attendance_status.c_str());
						}
					}
					
					if (event.unlock_method != unlock_method::UNLOCK_UNKNOWN) {
						list.push_back(event);
						log_debug(_device_sn, "\trecord(serialNo = %ld, time = %s, employeeNoString = %s, attendance = %d, unlock_method = %d)",
								 event.row_id, event.event_date.c_str(), event.user_id.c_str(), event.attendance, event.unlock_method);
					}
					else {
						log_debug(_device_sn, "\trecord(serialNo = %ld, time = %s, employeeNoString = %s, attendance = %d, item.minor = %d) skipped. unlock method is UNLOCK_UNKNOWN",
								  event.row_id,
								  event.event_date.c_str(),
								  event.user_id.c_str(),
								  event.attendance,
								  item.contains("minor") ? (int)item.at("minor") : -1);
					}
				}
				else {
					log_debug(_device_sn, "\trecord(serialNo = %ld, time = %s) skipped. \"employeeNoString\" field not found",
							 (long)item.at("serialNo"),
							 ((std::string)item.at("time")).c_str());
				}
				
				auto new_date = mylib::date((std::string)item.at("time")).set_timezone(start.timezone());
				auto current_date = next_event_date.set_timezone(start.timezone());
				if (new_date > current_date) {
					next_event_date = new_date;
				}
			}

			position += num_of_matches;
			has_more_data = status == "MORE";
		}
		catch (const std::exception& e) {
			log_error(_device_sn, "get_records(%s, %s): %s\n%s",
				start.iso_string().c_str(),
				end.iso_string().c_str(),
				e.what(),
				response.dump().c_str());
			break;
		}

	}

	log_debug(_device_sn, "get_records(%s, %s) end: total = %u, filtered = %u",
			 start.iso_string().c_str(), end.iso_string().c_str(),
			 position, list.size());
}

void device::set_attendance_mode(const std::string& mode, bool enable) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	if (mode != "disabled" && mode != "auto" &&
		mode != "manual" && mode != "manualAndAuto") {
		throw std::runtime_error("Invalid mode: " + mode);
	}

	nlohmann::json response = request::put(_conn_id, "/ISAPI/AccessControl/Configuration/attendanceMode?format=json", nlohmann::json({
		{"AttendanceMode", {
			{"mode", mode},
			{"reqAttendanceStatus", enable}
		}}
	}));
	request::check_response(response);
}

void device::set_attendance_status(const std::string& status, const std::string& label, bool enable) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	std::string key = "1";
	if (status == "checkIn") {
		key = "1";
	}
	else if (status == "checkOut") {
		key = "2";
	}
	else if (status == "breakOut") {
		key = "3";
	}
	else if (status == "breakIn") {
		key = "4";
	}
	else if (status == "overtimeIn") {
		key = "5";
	}
	else if (status == "overtimeOut") {
		key = "6";
	}
	else {
		throw std::runtime_error("Invalid status key: " + status);
	}

	nlohmann::json response = request::put(_conn_id, "/ISAPI/AccessControl/keyCfg/" + key + "/attendance?format=json", nlohmann::json({
		{"Attendance", {
			{"attendanceStatus", status},
			{"label", label},
			{"enable", enable}
		}}
	}));
	request::check_response(response);

}

void device::set_network(bool enable_dhcp, const std::string& ip, const std::string& mask, const std::string& gateway) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	pugi::xml_document doc;

	auto declaration = doc.append_child(pugi::node_declaration);
	declaration.append_attribute("version").set_value("1.0");
	declaration.append_attribute("encoding").set_value("UTF-8");

	auto network_interface = doc.append_child("NetworkInterface");
	network_interface
		.append_child("id")
		.append_child(pugi::node_pcdata)
		.set_value("1");

	auto ip_address = network_interface.append_child("IPAddress");
	ip_address
		.append_child("ipVersion")
		.append_child(pugi::node_pcdata)
		.set_value("v4");

	if (enable_dhcp) {
		ip_address
			.append_child("addressingType")
			.append_child(pugi::node_pcdata)
			.set_value("dynamic");
	}
	else {
		ip_address
			.append_child("addressingType")
			.append_child(pugi::node_pcdata)
			.set_value("static");
		
		ip_address
			.append_child("ipAddress")
			.append_child(pugi::node_pcdata)
			.set_value(ip.c_str());

		ip_address
			.append_child("subnetMask")
			.append_child(pugi::node_pcdata)
			.set_value(mask.c_str());

		ip_address
			.append_child("DefaultGateway")
			.append_child("ipAddress")
			.append_child(pugi::node_pcdata)
			.set_value(gateway.c_str());
	}

	ip_address.append_child("bitMask");
	ip_address
		.append_child("PrimaryDNS")
		.append_child("ipAddress")
		.append_child(pugi::node_pcdata)
		.set_value("8.8.8.8");
	
	ip_address
		.append_child("SecondaryDNS")
		.append_child("ipAddress")
		.append_child(pugi::node_pcdata)
		.set_value("8.8.4.4");

	auto response = request::put(_conn_id, "/ISAPI/System/Network/interfaces/1", doc);
	request::check_response(response);
}

void device::set_isup(bool enable, const std::string& device_id, const std::string server_ip, int server_port, std::string ehome_key) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	pugi::xml_document doc;

	auto declaration = doc.append_child(pugi::node_declaration);
	declaration.append_attribute("version").set_value("1.0");
	declaration.append_attribute("encoding").set_value("UTF-8");

	auto ehome = doc.append_child("Ehome");
	ehome.append_attribute("xmlns").set_value("http://www.isapi.org/ver20/XMLSchema");
	ehome.append_attribute("version").set_value("2.0");

	ehome.append_child("enabled").append_child(pugi::node_pcdata).set_value(enable ? "true" : "false");
	ehome.append_child("deviceID").append_child(pugi::node_pcdata).set_value(device_id.c_str());
	ehome.append_child("addressingFormatType").append_child(pugi::node_pcdata).set_value("ipaddress");
	ehome.append_child("ipAddress").append_child(pugi::node_pcdata).set_value(server_ip.c_str());
	ehome.append_child("portNo").append_child(pugi::node_pcdata).set_value(std::to_string(server_port).c_str());
	ehome.append_child("key").append_child(pugi::node_pcdata).set_value(ehome_key.c_str());
	ehome.append_child("protocolVersion").append_child(pugi::node_pcdata).set_value("v5.0");

	auto response = request::put(_conn_id, "/ISAPI/System/Network/Ehome", doc);
	request::check_response(response);
}

void device::set_timezone(short offset) {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}
	
	int hours = std::abs(offset / 60);
	int min = std::abs(offset) - hours * 60;
	char tz[16];
	sprintf(tz, "GMT%s%02d:%02d", offset < 0 ? "+" : "-", hours, min);

	pugi::xml_document doc;

	auto declaration = doc.append_child(pugi::node_declaration);
	declaration.append_attribute("version").set_value("1.0");
	declaration.append_attribute("encoding").set_value("UTF-8");

	auto time = doc.append_child("Time");
	time.append_attribute("xmlns").set_value("http://www.isapi.org/ver20/XMLSchema");
	time.append_attribute("version").set_value("2.0");
	time.append_child("timeMode").append_child(pugi::node_pcdata).set_value("manual");
	time.append_child("localTime").append_child(pugi::node_pcdata).set_value(mylib::date().set_timezone(offset).iso_string().c_str());
	time.append_child("timeZone").append_child(pugi::node_pcdata).set_value(tz);

	auto response = request::put(_conn_id, "/ISAPI/System/time", doc);
	request::check_response(response);
}

void device::reboot() {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}

	pugi::xml_document body;
	pugi::xml_document response = request::put(_conn_id, "/ISAPI/System/reboot", body);
	request::check_response(response);
}

void device::clear_data() {

	if (!_connected) {
		throw std::runtime_error("device is disconnected");
	}
	
	clear_users();
}

bool device::logout() {
	if (_connected) {
		_connected = false;
		return NET_ECMS_ForceLogout(_conn_id);
	}
	return true;
}

