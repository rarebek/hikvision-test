#pragma once

#include <string>
#include <ctime>

namespace mylib {

    class date {
	public:
		static short system_timezone();

    public:
        date();
        date(std::time_t time);
        date(const std::string& iso_date);
        date(int year, int month = 1, int day = 1, int hours = 0, int minutes = 0, int seconds = 0, int timeZone = INT32_MAX);

        int year() const { return _year; }
        int month() const { return _month; }
        int day() const { return _day; }
        int hours() const { return _hours; }
        int minutes() const { return _minutes; }
        int seconds() const { return _seconds; }
        int timezone() const { return _timezone; }
	
        date& add_years(short years);
        date& add_months(short months);
        date& add_days(short days);
        date& add_hours(short hours);
        date& add_minutes(short minutes);
        date& add_seconds(short seconds);
        date& set_timezone(short timezone);

		std::string iso_string(bool dont_use_z = false, bool dont_use_t = false, bool dont_show_offset = false) const;
        std::time_t utc_time() const;
		void validate();

		std::string format(const std::string& format);

    private:
        int _year = 0;
        int _month = 1;
        int _day = 1;
        int _hours = 0;
        int _minutes = 0;
        int _seconds = 0;
        int _timezone = INT32_MAX; // minutes
    };

    bool operator >= (const date& d1, const date& d2);
    bool operator <= (const date& d1, const date& d2);
    bool operator > (const date& d1, const date& d2);
    bool operator < (const date& d1, const date& d2);
    bool operator == (const date& d1, const date& d2);
    bool operator != (const date& d1, const date& d2);
}