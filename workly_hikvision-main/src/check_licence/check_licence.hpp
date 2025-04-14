#pragma once

#include <string>

namespace licence {

	void validate(std::string filename);

	std::string generate_temp_token();

}