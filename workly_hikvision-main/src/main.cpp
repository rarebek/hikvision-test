#include <iostream>
#include <string>
#include <cstring>
#include <csignal>
#include <condition_variable>
#include <mutex>

#include "config/config.hpp"
#include "db/db.hpp"
#include "logger/logger.hpp"
#include "cms_server/cms_server.hpp"
#include "http_server/http_server.hpp"
#include "check_licence/check_licence.hpp"
#include "ipc_server/ipc_server.hpp"

// #define __CHECK_LICENCE__

void generate_token(int argc, char** argv);
void verify_token(int argc, char** argv);
void read_config(int argc, char** argv);
void signal_handler(int signal);

using mylib::logger;
std::condition_variable _cv;
std::mutex _mtx;
cms_server _cms;
http_server _http;

int main(int argc, char** argv) {

	generate_token(argc, argv);
	read_config(argc, argv);
	verify_token(argc, argv);

	logger::instance().set_directory(config::logs_directory() + "cms/");
	logger::instance().set_enable_debug_logs(config::enable_debug_logs());
	logger::instance().set_cleaner_params(config::keep_logs_days(), config::check_logs_hours());
	logger::instance().start();
	db::instance().start();
	_http.start([](const http_request& request, http_response& response) {

		if (request.method() == "get") {
			if (request.path().find("/face?id=") == 0) {
				auto id = request.path().substr(9);
				int face_id = std::atol(id.c_str());
				std::vector<char> face_data;

				try {
					db::instance().get_face_data(face_id, face_data);
					response.set_status(200, "OK");
					response.set_header("Content-Type", "image/jpeg");
					response.set_body(std::move(face_data));
					response.end();
				}
				catch (const std::exception& e) {
					response.set_status(404, "Resource not found");
					response.end();
				}
			}
			else {
				response.set_status(404, "Route not found");
				response.end();
			}
		}
		else {
			response.set_status(405, "Method not allowed");
			response.end();
		}
	});
	ipc_server::instance().start(config::ipc_server_port());

	cms_server::init_sdk();
	if (_cms.start()) {
		std::signal(2, signal_handler);	 //	SIGINT
		std::signal(15, signal_handler); //	SIGTERM
		std::unique_lock<std::mutex> lock(_mtx);
		_cv.wait(lock);
	}

	_http.stop();
	_cms.stop();
	ipc_server::instance().stop();
	cms_server::destroy_sdk();
	db::instance().stop();
}

void read_config(int argc, char** argv) {
		
	try {
		std::string filename = "config.json";
		for (int i = 1; i + 1 < argc; i += 2) {
			if (std::strcmp(argv[i], "-c") == 0) {
				filename = argv[i + 1];
			}
		}
		config::parse_from(filename);
	}
	catch (const std::exception& e) {
		std::cerr << "read_config(): " << e.what() << std::endl;
		exit(-1);
	}
	
}

void generate_token(int argc, char** argv) {

	#ifdef __CHECK_LICENCE__
 	for (int i = 1; i < argc; ++i) {
 		if (std::strcmp(argv[i], "auth") == 0) {
 			try {
 				std::string token = licence::generate_temp_token();
 				std::cout << token << std::endl;
 			}
 			catch (const std::exception& e) {
 				std::cerr << e.what() << std::endl;
			}
			exit(0);
		}
	}
	#endif
}

void verify_token(int argc, char** argv) {
	
	#ifdef __CHECK_LICENCE__
	if (config::token_filename().empty()) {
		std::cerr << "\"token_file\" is required in \"config.json\"" << std::endl;
		exit(-3);
	}
	try {
		licence::validate(config::token_filename());
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return exit(-4);
	}
	#endif
}

void signal_handler(int signal) {
	_cv.notify_one();
}