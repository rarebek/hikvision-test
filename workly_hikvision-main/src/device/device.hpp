#pragma once

#include <string>
#include <vector>
#include <list>
#include <functional>
#include <chrono>
#include "../date/date.hpp"
#include "../db/db_types.hpp"
#include "../../libisup/include/HCISUPCMS.h"
#include "./../libisup/include/HCISUPAlarm.h"
#include "./../libisup/include/HCISUPSS.h"

enum MajorTypes {
	MAJOR_ALARM = 0x01,
	MAJOR_EXCEPTION = 0x02,
	MAJOR_OPERATION = 0x03,
	MAJOR_EVENT = 0x05,
};

enum MinorTypes_MajorEvent {
	MINOR_LEGAL_CARD_PASS = 0x01, 						// Valid Card Authentication Completed
	MINOR_CARD_AND_PSW_PASS = 0x02, 					// Card and Password Authentication Completed
	MINOR_CARD_FINGERPRINT_VERIFY_PASS = 0x28,			// Card and Fingerprint Authentication Completed
	MINOR_FINGERPRINT_COMPARE_PASS = 0x26,				// Fingerprint Matched
	MINOR_CARD_FINGERPRINT_PASSWD_VERIFY_PASS = 0x2b, 	// Card and Fingerprint and Password Authentication Completed
	MINOR_FINGERPRINT_PASSWD_VERIFY_PASS = 0x2e, 		// Fingerprint and Password Authentication Completed
	MINOR_EMPLOYEENO_AND_FP_VERIFY_PASS = 0x45, 		// Employee ID and Fingerprint Authentication Completed
	MINOR_EMPLOYEENO_AND_FP_AND_PW_VERIFY_PASS = 0x48, 	// Employee ID and Fingerprint and Password Authentication Completed
	MINOR_FACE_VERIFY_PASS = 0x4b, 						// Face Authentication Completed			
	MINOR_EMPLOYEENO_AND_FACE_VERIFY_PASS = 0x4d, 		// Employee ID and Face Authentication Completed
	MINOR_EMPLOYEENO_AND_PW_PASS = 0x65, 				// Employee ID and Password Authentication Completed
	MINOR_PASSWD_VERIFY_PASS = 0xb5,					// Password Authenticated

	MINOR_LOCK_OPEN = 0x15,								// Door unlocked
	MINOR_LOCK_CLOSE = 0x16,							// Door locked
	MINOR_FACE_VERIFY_FAIL = 0x4c,						// Face Authentication Failed
};

class device {
public:
	device(long conn_id, std::string device_sn, std::string device_id, std::string device_ip, unsigned short device_port);
	~device();

	void add_user(std::string user_id, std::string name, std::string password, bool is_admin);
	void del_user(std::string user_id);
	void get_users(std::list<device_user>& buffer);
	void clear_users();

	void add_card(const std::string& user_id, const std::string& card_no);
	void del_card(const std::string& card_no);
	void del_user_cards(const std::string& user_id);
	void get_cards(const std::string& user_id, std::list<std::string>& buffer);

	void set_face_url(const std::string& user_id, const std::string& face_url);
	void get_face_url(const std::string& user_id, std::string& face_url);
	void del_facedata(const std::string& user_id);

	void set_fingerprint(const std::string& user_id, int fp_id, const std::string& b64_fp_data);
	void get_fingerprints(const std::string& user_id, std::list<std::pair<std::string, int>>& fp_list);
	void del_fingerprint(const std::string& user_id, int fp_id);

	void get_records(mylib::date start, mylib::date end, std::list<device_event>& buffer, mylib::date& next_event_date);
	// void del_records(mylib::date start, mylib::date end);	not available
	// void clear_records();	not available

	void set_attendance_mode(const std::string& mode, bool enable);
	void set_attendance_status(const std::string& status, const std::string& label, bool enable);

	void set_network(bool enable_dhcp, const std::string& ip, const std::string& mask, const std::string& gateway);
	void set_isup(bool enable, const std::string& device_id, const std::string server_ip, int server_port, std::string ehome_key);
	void set_timezone(short minutes);
	void reboot();
	void clear_data();
	bool logout();

	const int64_t& conn_id() const { return _conn_id; }
	const std::string& serial_number() const { return _device_sn; }
	const std::string& name() const { return _device_id; }
	const std::string& ip() const { return _device_ip; }
	const unsigned short& port() const { return _device_port; }

private:
	std::string _device_id;
	std::string _device_sn;
	std::string _device_ip;
	unsigned short _device_port;
	long _conn_id;
	short _timezone;
	std::string _blackfd;
	bool _connected;

	const int TIMEOUT = 10000;
	const int RETRY = 3;
	const std::chrono::milliseconds RETRY_TIMEOUT = std::chrono::milliseconds(400);

private:
	bool is_user_exist(std::string user_id);
	std::string get_facelib_id(std::string facelib);
	std::string get_blackfd();
};
