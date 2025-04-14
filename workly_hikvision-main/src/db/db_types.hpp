#pragma once

#include <string>

struct device_command {
	long long id;
	std::string data;
};

struct command_status {
	int64_t cmd_id;
	bool success;
	std::string error;
	std::string transfer_time;
	std::string over_time;
};

enum class device_mode {
	check_in,
	check_out,
	both
};

struct device_info {
	long company_id;
	long device_id;
	long card_type_id;
	device_mode mode;
	short timezone;
	std::string last_inout_time;
	std::string device_sn;
};

enum attendance {
	ATTENDANCE_CHECKIN = 0,
	ATTENDANCE_CHECKOUT = 1,
	ATTENDANCE_UNKNOWN = 100
};

enum unlock_method {
	UNLOCK_PASSWORD = 0,
	UNLOCK_FINGERPRINT = 1,
	UNLOCK_CARD = 2,
	UNLOCK_FACE = 3,
	UNLOCK_UNKNOWN = 100
};

struct device_event {
	long row_id;
	std::string user_id;
	std::string event_date;
	std::string request_date;
	int unlock_method;
	int attendance; 	// 0: check-in, 1: check-out
};

struct device_user {
	std::string user_id;
	std::string name;
	std::string password;
};