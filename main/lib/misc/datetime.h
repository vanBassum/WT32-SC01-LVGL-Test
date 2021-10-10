/*
 * DateTime.h
 *
 *  Created on: 9 Sep 2020
 *      Author: Bas
 */

#ifndef COMPONENTS_CPP_LIB_MISC_DATETIME_H_
#define COMPONENTS_CPP_LIB_MISC_DATETIME_H_

#include <time.h>
#include "TimeSpan.h"
#include <string>

class DateTime
{

public:

	time_t unixTime = 0;

	void Clear()
	{
		unixTime = 0;
	}

	static DateTime FromUTC(uint64_t u)
	{
		DateTime dt;
		dt.unixTime = u;
		return dt;
	}

	static DateTime Now()
	{
		DateTime dt;
		dt.unixTime = time(NULL);
		return dt;
	}

	static DateTime FromLocalTime(struct tm tm)
	{
		DateTime dt;
		dt.unixTime = mktime(&tm);
		return dt;
	}

	struct tm GetLocalTime()
	{
		struct tm timeinfo;
		timeinfo = *localtime(&unixTime);
		return timeinfo;
	}


	std::string ToString()
	{
		struct tm timeinfo;
		char buffer[80];

		timeinfo = GetLocalTime();

		strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", &timeinfo);
		return std::string(buffer);
	}




/*
	time_t MKTime() const
	{
		struct tm tm = _time;
		return mktime(&tm);
	}



	static DateTime Empty()
	{
		time_t t = 0;
		struct tm *now = localtime(&t);
		return FromTM(*now);
	}

	static DateTime FromTM(struct tm tm)
	{
		DateTime dt;
		dt._time.tm_sec   = tm.tm_sec  ;
		dt._time.tm_min   = tm.tm_min  ;
		dt._time.tm_hour  = tm.tm_hour ;
		dt._time.tm_mday  = tm.tm_mday ;
		dt._time.tm_mon   = tm.tm_mon  ;
		dt._time.tm_year  = tm.tm_year ;
		dt._time.tm_wday  = tm.tm_wday ;
		dt._time.tm_yday  = tm.tm_yday ;
		dt._time.tm_isdst = tm.tm_isdst;
		return dt;
	}

	static DateTime FromTimeT(time_t t)
	{
		struct tm *tm = localtime(&t);
		return FromTM(*tm);
	}

	DateTime AddSeconds(time_t seconds)	//seconds is the right hand side
	{
		time_t t = MKTime() + seconds;
		return FromTimeT(t);
	}

	static bool TryParse(std::string raw, DateTime *val)
	{
		return strptime(raw.c_str(), DATETIEMFORMAT, &val->_time) != NULL;	//TODO not completely safe. see https://man7.org/linux/man-pages/man3/strptime.3.html#RETURN_VALUE
	}

	std::string ToString()
	{
		char buff[64];
		strftime(buff, 64, DATETIEMFORMAT, &_time);
		return buff;
	}

*/
};



inline bool operator==(DateTime const &lhs, DateTime const &rhs) { return difftime(lhs.unixTime, rhs.unixTime) == 0; }
inline bool operator!=(DateTime const &lhs, DateTime const &rhs) { return difftime(lhs.unixTime, rhs.unixTime) != 0; }
inline bool operator< (DateTime const &lhs, DateTime const &rhs) { return difftime(lhs.unixTime, rhs.unixTime) <  0; }
inline bool operator> (DateTime const &lhs, DateTime const &rhs) { return difftime(lhs.unixTime, rhs.unixTime) >  0; }
inline bool operator<=(DateTime const &lhs, DateTime const &rhs) { return difftime(lhs.unixTime, rhs.unixTime) <= 0; }
inline bool operator>=(DateTime const &lhs, DateTime const &rhs) { return difftime(lhs.unixTime, rhs.unixTime) >= 0; }


inline TimeSpan operator-(DateTime const &lhs, DateTime const &rhs) { return TimeSpan::FromSeconds(lhs.unixTime - rhs.unixTime); }
inline DateTime operator+(DateTime const &lhs, TimeSpan const &rhs) { return DateTime::FromUTC(lhs.unixTime + rhs.ticks); }



#endif /* COMPONENTS_CPP_LIB_MISC_DATETIME_H_ */
