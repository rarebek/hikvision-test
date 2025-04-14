#include "db.hpp"
#include "../config/config.hpp"
#include "../encoders/base64/base64.hpp"
#include <stdexcept>
#include <cstring>

db::db() {}

db::~db() {
	stop();
}

db& db::instance() {
	static db _instance;
	return _instance;
}

void db::start() {
    
	_service.reset(new async_pg({
		{"hostaddr", config::db_address()},
		{"port", config::db_port()},
		{"dbname", config::db_name()},
		{"user", config::db_user()},
		{"password", config::db_password()},
		{"sslmode", config::db_ssl_mode()}
	}));

	_service->start(10);
}

void db::stop() {
    _service->stop();
}


void db::get_device_info(const std::string& device_sn, device_info& info) {
    
    std::list<pg_param> params;
	params.push_back(pg_param::text(device_sn));

	auto results = _service->execute_prepared("get_device_info", R"(
        SELECT dev.id, dev.in_mode, dev.out_mode, dev.time_zone_adj, dev.company_id, dev.last_inout_time, COALESCE(card.card_type_id, 0) AS card_type_id
        FROM w_device AS dev
        LEFT JOIN w_device_card_type AS card
	        ON dev.id = card.device_id
        WHERE dev.serial_number=$1::varchar(250)
    )", std::move(params)).get();

	auto result = std::move(results.front());

	result.check();
    if (result.rows_count() == 0) {
        throw std::runtime_error("Device not found");
    }
    
    int f_device_id = result.col_number("id");
    int f_in_mode = result.col_number("in_mode");
    int f_out_mode = result.col_number("out_mode");
    int f_time_zone = result.col_number("time_zone_adj");
    int f_company_id = result.col_number("company_id");
    int f_last_inout_time = result.col_number("last_inout_time");
    int f_card_type = result.col_number("card_type_id");

    info.device_id = std::atol(result.get_value(0, f_device_id));
    info.timezone = std::atol(result.get_value(0, f_time_zone));
    info.company_id = std::atol(result.get_value(0, f_company_id));
    info.last_inout_time = result.get_value(0, f_last_inout_time);
    info.card_type_id = std::atol(result.get_value(0, f_card_type));
    bool in_mode = strcmp(result.get_value(0, f_in_mode), "t") == 0;
	bool out_mode = strcmp(result.get_value(0, f_out_mode), "t") == 0;
	if (in_mode && out_mode) {
        info.mode = device_mode::both;
    }
    else if (in_mode) {
        info.mode = device_mode::check_in;
    }
    else if (out_mode) {
        info.mode = device_mode::check_out;
	}
	else {
		info.mode = device_mode::both;
	}
	info.device_sn = device_sn;
}

void db::get_commands(const std::string& device_sn, std::list<device_command>& cmds) {
    
    std::list<pg_param> params;
    params.push_back(pg_param::text(device_sn));
    auto results = _service->execute_prepared("get_commands", R"(
        SELECT
            c.id as command_id,
            c.cmd_content as command_data
        FROM w_device_cmds AS c
        INNER JOIN w_device d
            ON d.id = c.device_id
        WHERE
            c.cmd_status IS NULL AND
            c.cmd_content!='CHECK' AND
            d.device_type_id=37 AND
            d.serial_number=$1::varchar(250)
        ORDER BY c.id ASC LIMIT 100
    )", std::move(params)).get();

	auto result = std::move(results.front());
	result.check();
    if (result.rows_count() == 0) {
        return;
    }

    int f_command_id = result.col_number("command_id");
    int f_command_data = result.col_number("command_data");
    for( int i = 0; i < result.rows_count(); ++i ) {
        device_command cmd;
        cmd.id = std::atoll(result.get_value(i, f_command_id));
        cmd.data = result.get_value(i, f_command_data);
        cmds.push_back(cmd);
    }
}

void db::get_face_data(int face_id, std::vector<char>& face_data) {
    
    std::list<pg_param> params;
    params.push_back(pg_param::int32(face_id));
	auto results = _service->execute_prepared("get_face_data",
											"SELECT encode(face, 'base64') AS data FROM w_faces WHERE id=$1::integer",
											  std::move(params)).get();

	auto result = std::move(results.front());
	result.check();
    if (result.rows_count() == 0) {
        throw std::runtime_error("Requested face data (id=" + std::to_string(face_id) + ") not found");
    }

    face_data = base64_decode(result.get_value(0, result.col_number("data")));
}


void db::write_fingerprint_data(int64_t employee_id, int64_t finger_index,  std::vector<char>&& data) {

    std::list<pg_param> params;
    params.push_back(pg_param::int64(employee_id));
    params.push_back(pg_param::int64(finger_index));
    params.push_back(pg_param::blob(data));
    auto results = _service->execute_prepared("update_fingerprint_data", R"(
        UPDATE w_fingerprint
        SET fingerprint=$3::bytea
        WHERE
            employee_id=$1::bigint AND finger=$2::bigint AND devicegroup_id=6;
    )", std::move(params)).get();

	auto result = std::move(results.front());
	result.check();
    if (result.rows_affected() == 0) {
        params.push_back(pg_param::int64(employee_id));
        params.push_back(pg_param::int64(finger_index));
        params.push_back(pg_param::blob(data));
		results = _service->execute_prepared(
			"write_fingerprint_data",
			R"(
				INSERT INTO w_fingerprint
					(employee_id, finger, fingerprint, devicegroup_id)
				VALUES
					($1::bigint, $2::bigint, $3::bytea, 6)
			)",
			std::move(params)).get();

		result = std::move(results.front());
		result.check();
    }
}

void db::write_face_data(int64_t persoin_id, std::vector<char>&& data) {
    
    std::list<pg_param> params;
    params.push_back(pg_param::int64(persoin_id));
    params.push_back(pg_param::blob(data));
	auto results = _service->execute_prepared(
		"update_face_data",
		"UPDATE w_faces SET face=$2::bytea WHERE person_id=$1::bigint AND devicegroup_id=6",
		std::move(params)).get();

	auto result = std::move(results.front());
	result.check();
	if (result.rows_affected() == 0) {
		params.clear();
		params.push_back(pg_param::int64(persoin_id));
        params.push_back(pg_param::blob(data));
		results = _service->execute_prepared(
			"write_face_data",
			R"(
				INSERT INTO w_faces
					(person_id, face, devicegroup_id)
				VALUES
					($1::bigint, $2::bytea, 6)
			)",
			std::move(params)).get();

		result = std::move(results.front());
		result.check();
    }
}

void db::write_card_data(int64_t employee_id, long company_id, long card_type_id, const std::string& card) {
    std::list<pg_param> params;
    params.push_back(pg_param::int64(employee_id));
    params.push_back(pg_param::int32(company_id));
    params.push_back(pg_param::int32(card_type_id));
    params.push_back(pg_param::text(card));
	auto results = _service->execute_prepared(
		"write_card_data",
		R"(
			INSERT INTO w_card
				(employee_id, company_id, card_type_id, code, state_col)
			VALUES
				($1::bigint, $2::int, $3::int, $4::varchar, true)
		)",
		std::move(params)).get();

	auto result = std::move(results.front());
	result.check();
}

void db::write_record(const std::string& device_sn, const device_event& record) {
    std::list<pg_param> params;
    params.push_back(pg_param::text(device_sn));
    params.push_back(pg_param::int32(std::atoi(record.user_id.c_str())));
    params.push_back(pg_param::text(record.event_date));
    params.push_back(pg_param::int16(record.attendance));
    params.push_back(pg_param::int16(record.unlock_method));
    params.push_back(pg_param::text(record.request_date));
	auto results = _service->execute_prepared(
		"write_record",
		R"(
			INSERT INTO w_temp_inout
				(device_sn, pin, time_col, status, verify, at_col)
			VALUES
				($1::varchar(250), $2::integer, $3::timestamp,
				$4::smallint, $5::smallint, $6::timestamp)
		)",
		std::move(params)).get();

	auto result = std::move(results.front());
	result.check();
}

void db::write_command_status(const command_status& cmd_sts) {
    std::list<pg_param> params;
    params.push_back(pg_param::boolean(cmd_sts.success));
    params.push_back(pg_param::text(cmd_sts.transfer_time));
    params.push_back(pg_param::text(cmd_sts.over_time));
    params.push_back(pg_param::text(cmd_sts.error));
    params.push_back(pg_param::int64(cmd_sts.cmd_id));
	auto results = _service->execute_prepared(
		"write_command_status",
		R"(
			UPDATE w_device_cmds
			SET
				cmd_status=$1::boolean,
				command_transfer_time=$2::timestamp,
				command_over_time=$3::timestamp,
				error_description=$4::varchar(255)
			WHERE id=$5::bigint
		)",
		std::move(params)).get();

	auto result = std::move(results.front());
	result.check();
}


bool db::update_last_request_time(const std::string& device_sn, const std::string& timestamp, const std::string& ip, const unsigned short& port) {
	std::list<pg_param> params;
    params.push_back(pg_param::text(device_sn));
	params.push_back(pg_param::text(timestamp));
	params.push_back(pg_param::text(ip));
	params.push_back(pg_param::int32(port));
	auto results = _service->execute_prepared(
		"update_last_request_time",
		R"(
			UPDATE w_device
				SET last_request_time=$2::timestamp, ip = $3, port = $4
			WHERE serial_number=$1::varchar(250)
			RETURNING id
		)",
		std::move(params)).get();

	auto result = std::move(results.front());
	result.check();
    return result.rows_count() > 0;
}

bool db::update_last_inout_time(const std::string& device_sn, std::string timestamp) {

    std::list<pg_param> params;
    params.push_back(pg_param::text(device_sn));
    params.push_back(pg_param::text(timestamp));
	auto results = _service->execute_prepared(
		"update_last_inout_time",
		R"(
			UPDATE w_device
			SET last_inout_time=$2::timestamp
			WHERE serial_number=$1::varchar(250)
			RETURNING id
		)",
		std::move(params)).get();

	auto result = std::move(results.front());
	result.check();
    return result.rows_count() > 0;
}

bool db::update_device_name(const std::string& device_sn, const std::string& device_name) {

    std::list<pg_param> params;
    params.push_back(pg_param::text(device_sn));
    params.push_back(pg_param::text(device_name));
	auto results = _service->execute_prepared(
		"update_device_name",
		R"(
			UPDATE w_device
			SET alias=$2::varchar(100)
			WHERE serial_number=$1::varchar(250)
			RETURNING id
		)",
		std::move(params)).get();

	auto result = std::move(results.front());
	result.check();
    return result.rows_count() > 0;
}

void db::delete_cards(int64_t employee_id, long company_id) {
    std::list<pg_param> params;
    params.push_back(pg_param::int64(employee_id));
    params.push_back(pg_param::int32(company_id));
	auto&& results = _service->execute_prepared(
		"delete_cards",
		R"(
			DELETE FROM w_card
			WHERE employee_id=$1::bigint AND company_id=$2::int
		)",
		std::move(params)).get();

	auto result = std::move(results.front());
	result.check();
}
