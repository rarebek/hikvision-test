#pragma once

#include <string>
#include <list>
#include <vector>
#include <mutex>
#include <memory>
#include "db_types.hpp"
#include "async_pg/async_pg.hpp"

class db {
	db();
	~db();
public:
	static db& instance();
	
	void start();
    void stop();
    
    void get_device_info(const std::string& device_sn, device_info& info);
    void get_commands(const std::string& device_sn, std::list<device_command>& cmds);
    void get_face_data(int face_id, std::vector<char>& face_data);
    
    void write_fingerprint_data(int64_t employee_id, int64_t finger_index, std::vector<char>&& data);
    void write_face_data(int64_t persoin_id, std::vector<char>&& face_data);
    void write_card_data(int64_t employee_id, long company_id, long card_type_id, const std::string& card);
    void write_record(const std::string& device_sn, const device_event& record);
    void write_command_status(const command_status& status);
    
    bool update_last_request_time(const std::string& device_sn, const std::string& timestamp, const std::string& ip, const unsigned short& port);
    bool update_last_inout_time(const std::string& device_sn, std::string timestamp);
    bool update_device_name(const std::string& device_sn, const std::string& device_name);

    void delete_cards(int64_t employee_id, long company_id);

private:
	std::unique_ptr<async_pg> _service;
};
