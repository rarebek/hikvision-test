#include "date.hpp"
#include <stdexcept>
#include <cstring>

namespace mylib {

    bool is_leap_year(int year) {
        if (year % 4 != 0) {
            return false;
        }
        else if (year % 100 != 0) {
            return true;
        }
        else if (year % 400 != 0) {
            return false;
        }
        else {
            return true;
        }
    }

    int get_days_in_month(int year, int month) {
        constexpr int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (month == 2 && is_leap_year(year)) {
            return 29;
        }
        return days[month - 1];
    }

    short date::system_timezone() {
        static short timezone = INT16_MAX;
        if (timezone == INT16_MAX) {
            std::time_t utc = std::time(nullptr);
            struct tm tm = *gmtime(&utc);
            std::time_t local = mktime(&tm);
            timezone = static_cast<short>((utc - local) / 60);
        }
		return timezone;
    }

    date::date(): date(std::time(nullptr)) {}

    date::date(std::time_t time) {
        struct tm utc = *gmtime(&time);
        _year = utc.tm_year + 1900;
        _month = utc.tm_mon + 1;
        _day = utc.tm_mday;
        _hours = utc.tm_hour;
        _minutes = utc.tm_min;
        _seconds = utc.tm_sec;
        _timezone = 0;
    }

    date::date(const std::string& iso_date) {

        // YYYY
        if (iso_date.length() == 4) {
            sscanf(iso_date.c_str(), "%4d", &_year);
        }
        // YYYY-MM
        else if (iso_date.length() == 7) {
            sscanf(iso_date.c_str(), "%4d-%2d", &_year, &_month);
        }
        //    YYYY-MM-DD
        else if (iso_date.length() == 10) {
            sscanf(iso_date.c_str(), "%4d-%2d-%2d", &_year, &_month, &_day);
        }
        //    YYYY-MM-DDThh
        else if (iso_date.length() == 13) {
            char divider;
            sscanf(iso_date.c_str(), "%4d-%2d-%2d%c%2d", &_year, &_month, &_day, &divider, &_hours);
        }
        //    YYYY-MM-DDThh:mm
        else if (iso_date.length() == 16) {
            char divider;
            sscanf(iso_date.c_str(), "%4d-%2d-%2d%c%2d:%2d", &_year, &_month, &_day, &divider, &_hours, &_minutes);
        }
        //    YYYY-MM-DDThh:mm:ssTIMEZONE_OFFSET
        else if (iso_date.length() >= 19) {
            char divider;
            char tz[10] = "";
            sscanf(iso_date.c_str(), "%4d-%2d-%2d%c%2d:%2d:%2d%s", &_year, &_month, &_day, &divider, &_hours, &_minutes, &_seconds, tz);

            // parse timezone
            if (strcmp(tz, "Z") == 0) {
                _timezone = 0;
            }
            else if (strlen(tz) > 0) {
                char sign = ' ';
                int hh = 0, mm = 0;
                sscanf(tz, "%c%02d:%02d", &sign, &hh, &mm);
                _timezone = hh * 60 + mm;
                if (sign != '+') {
                    _timezone = -_timezone;
                }
            }
        }

		validate();
    }

    date::date(int year, int month, int day, int hours, int minutes, int seconds, int timezone) {
        _year = year;
        _month = month;
        _day = day;
        _hours = hours;
        _minutes = minutes;
        _seconds = seconds;
        _timezone = timezone;
    }

	void date::validate() {
		if (_month > 12 || _month < 1) {
            throw std::invalid_argument("Invalid month");
        }
        else if (_day > get_days_in_month(_year, _month) || _day < 1) {
            throw std::invalid_argument("Invalid day");
        }
        else if (_hours > 23 || _hours < 0) {
            throw std::invalid_argument("Invalid hours");
        }
        else if (_minutes > 59 || _minutes < 0) {
            throw std::invalid_argument("Invalid minutes");
        }
        else if (_seconds > 59 || _seconds < 0) {
            throw std::invalid_argument("Invalid seconds");
        }
        else if (_timezone != INT32_MAX && (_timezone < -1500 || _timezone > 1500)) {
            throw std::invalid_argument("Invalid timezone");
        }
	}

    date& date::add_years(short years) {
        _year += years;
        return *this;
    }

    date& date::add_months(short months) {

        _month += months;
        if (_month > 12) {
            short years = _month / 12;
            _month -= years * 12;
            return add_years(years);
        }
        else if (_month < 1) {
            short years = 1 - _month / 12;
            _month += years * 12;
            return add_years(-years);
        }
        return *this;
    }

    date& date::add_days(short days) {

        _day += days;
        int days_in_month = get_days_in_month(_year, _month);
        if (_day > days_in_month) {
            days = _day - days_in_month - 1;
            _day = 1;
            return add_months(1).add_days(days);
        }
        else if (_day < 1) {
            add_months(-1);
            days = get_days_in_month(_year, _month) + _day - 1;
            _day = 1;
            return add_days(days);
        }

        return *this;
    }

    date& date::add_hours(short hours) {
        _hours += hours;
        if (_hours > 23) {
            int days = _hours / 24;
            return add_hours(-days * 24).add_days(days);
        }
        else if (_hours < 0) {
            int days = 1 - _hours / 24;
            return add_hours(days * 24).add_days(-days);
        }

        return *this;
    }

    date& date::add_minutes(short minutes) {
        _minutes += minutes;
        if (_minutes > 59) {
            int hours = _minutes / 60;
            return add_minutes(-hours * 60).add_hours(hours);
        }
        else if (_minutes < 0) {
            int hours = 1 - _minutes / 60;
            return add_minutes(hours * 60).add_hours(-hours);
        }

        return *this;
    }

    date& date::add_seconds(short seconds) {

        _seconds += seconds;
        if (_seconds > 59) {
            int minutes = _seconds / 60;
            return add_seconds(-minutes * 60).add_minutes(minutes);
        }
        else if (_seconds < 0) {
            int minutes = 1 - _seconds / 60;
            return add_seconds(minutes * 60).add_minutes(-minutes);
        }

        return *this;
    }

    date& date::set_timezone(short new_timezone) {

        if (_timezone == INT32_MAX || _timezone == new_timezone) {
            _timezone = new_timezone;
            return *this;
        }
        else if (_timezone == 0) {
            _timezone = new_timezone;
            return add_minutes(new_timezone);
        }
        else {
            int minutes = new_timezone - _timezone;
            _timezone = new_timezone;
            return add_minutes(minutes);
        }

    }

    std::string date::iso_string(bool dont_use_z, bool dont_use_t, bool dont_show_offset) const {
		
		char delimiter = dont_use_t ? ' ' : 'T';

		if (_timezone == INT32_MAX || dont_show_offset) {
            char date[21];
            sprintf(date, "%04d-%02d-%02d%c%02d:%02d:%02d",
                _year, _month, _day, delimiter, _hours, _minutes, _seconds);
            return date;
        }
		else if (_timezone == 0 && !dont_use_z) {
			char date[21];
            sprintf(date, "%04d-%02d-%02d%c%02d:%02d:%02dZ",
                _year, _month, _day, delimiter, _hours, _minutes, _seconds);
            return date;
		}
		else {
            const int hh = std::abs(_timezone) / 60;
            const int mm = std::abs(_timezone) - hh * 60;
            char date[30];
            sprintf(date, "%04u-%02u-%02u%c%02u:%02u:%02u%c%02d:%02d",
                _year, _month, _day, delimiter, _hours, _minutes, _seconds,
                _timezone < 0 ? '-' : '+', hh, mm);
            return date;
        }
	}

	std::string date::format(const std::string& format) {

		char temp[20];
		std::string str;
		
		for (auto& c : format) {
			switch (c) {
				case 'Y':
					sprintf(temp, "%04d", _year);
					str += temp;
					break;

				case 'm':
					sprintf(temp, "%02d", _month);
					str += temp;
					break;

				case 'd':
					sprintf(temp, "%02d", _day);
					str += temp;
					break;

				case 'H':
					sprintf(temp, "%02d", _hours);
					str += temp;
					break;

				case 'i':
					sprintf(temp, "%02d", _minutes);
					str += temp;
					break;

				case 's':
					sprintf(temp, "%02d", _seconds);
					str += temp;
					break;

				default:
					str += c;
					break;
			}
		}

		return str;
	}

    std::time_t date::utc_time() const {

        date utc = *this;
        utc.set_timezone(0);

        struct tm tm = {};
        tm.tm_year = utc.year() - 1900;
        tm.tm_mon = utc.month() - 1;
        tm.tm_mday = utc.day();
        tm.tm_hour = utc.hours();
        tm.tm_min = utc.minutes();
        tm.tm_sec = utc.seconds();
        return mktime(&tm) + system_timezone() * 60;
    }

    bool operator>=(const date& d1, const date& d2) {
        return d1 == d2 || d1 > d2;
    }

    bool operator<=(const date& d1, const date& d2) {
        return d1 == d2 || d1 < d2;
    }

    bool operator>(const date& d1, const date& d2) {
        return d1 != d2 && !(d1 < d2);
    }

    bool operator<(const date& d1, const date& d2) {

        // compare them in the same timezone
        if (d1.timezone() == d2.timezone()) {

            const date& left = d1;
            const date& right = d2;

            if (left.year() != right.year()) {
                return left.year() < right.year();
            }
            else if (left.month() != right.month()) {
                return left.month() < right.month();
            }
            else if (left.day() != right.day()) {
                return left.day() < right.day();
            }
            else if (left.hours() != right.hours()) {
                return left.hours() < right.hours();
            }
            else if (left.minutes() != right.minutes()) {
                return left.minutes() < right.minutes();
            }
            else if (left.seconds() != right.seconds()) {
                return left.seconds() < right.seconds();
            }
        }
        else {
            date left = d1;
            date right = d2;

            if (left.timezone() == INT32_MAX) {
                left.set_timezone(right.timezone());
            }
            else {
                right.set_timezone(left.timezone());
            }
			return left < right;
        }

        return false;
    }

    bool operator==(const date& d1, const date& d2) {

        // compare both of them the same timezone
        if (d1.timezone() == d2.timezone()) {
            return
                d1.year() == d2.year() &&
                d1.month() == d2.month() &&
                d1.day() == d2.day() &&
                d1.hours() == d2.hours() &&
                d1.minutes() == d2.minutes() &&
                d1.seconds() == d2.seconds();
        }

        date left = d1;
        date right = d2;
        if (left.timezone() == INT32_MAX) {
            left.set_timezone(right.timezone());
        }
        else {
            right.set_timezone(left.timezone());
        }

        return left == right;
    }

    bool operator!=(const date& d1, const date& d2) {
        return !(d1 == d2);
    }

}
