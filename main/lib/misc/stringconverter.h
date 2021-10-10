/*
 * StringConverter.h
 *
 *  Created on: 9 Sep 2020
 *      Author: Bas
 */

#pragma once

#include <string>
#include <stdint.h>
#include <math.h>
#include "datetime.h"
#include "guid.h"
#include "version.h"
#include "esp_log.h"

class StringConverter
{

public:

	/*  ----------------------------  */
	/*	----------  8-bit ----------  */
	/*  ----------------------------  */


	static bool TryParse(std::string str, int8_t *val)
	{
		int64_t i;
		bool success = TryParse(str, &i);
		if(success)
			*val = i;
		return success;
	}

	static bool ToString(std::string *str, int8_t *val)
	{
		char buff[16];
		sprintf(buff, "%d", *val);
		*str = buff;
		return true;
	}


	static bool TryParse(std::string str, uint8_t *val)
	{
		uint64_t i;
		bool success = TryParse(str, &i);
		if(success)
			*val = i;
		return success;
	}

	static bool ToString(std::string *str, uint8_t *val)
	{
		char buff[16];
		sprintf(buff, "%d", *val);
		*str = buff;
		return true;
	}


	/*  ----------------------------  */
	/*	---------- 16-bit ----------  */
	/*  ----------------------------  */

	static bool TryParse(std::string str, int16_t *val)
	{
		int64_t i;
		bool success = TryParse(str, &i);
		if(success)
			*val = i;
		return success;
	}

	static bool ToString(std::string *str, int16_t *val)
	{
		char buff[16];
		sprintf(buff, "%d", *val);
		*str = buff;
		return true;
	}


	static bool TryParse(std::string str, uint16_t *val)
	{
		uint64_t i;
		bool success = TryParse(str, &i);
		if(success)
			*val = i;
		return success;
	}

	static bool ToString(std::string *str, uint16_t *val)
	{
		char buff[16];
		sprintf(buff, "%d", *val);
		*str = buff;
		return true;
	}


	/*  ----------------------------  */
	/*	---------- 32-bit ----------  */
	/*  ----------------------------  */

	static bool TryParse(std::string str, int32_t *val)
	{
		int64_t i;
		bool success = TryParse(str, &i);
		if(success)
			*val = i;
		return success;
	}

	static bool ToString(std::string *str, int32_t *val)
	{
		char buff[16];
		sprintf(buff, "%d", *val);
		*str = buff;
		return true;
	}


	static bool TryParse(std::string str, uint32_t *val)
	{
		uint64_t i;
		bool success = TryParse(str, &i);
		if(success)
			*val = i;
		return success;
	}

	static bool ToString(std::string *str, uint32_t *val)
	{
		char buff[16];
		sprintf(buff, "%d", *val);
		*str = buff;
		return true;
	}


	/*  ----------------------------  */
	/*	---------- 64-bit ----------  */
	/*  ----------------------------  */

	static bool TryParse(std::string str, int64_t *val)
	{
		//Explicitly chosen to not use the KC_functions.
		//We need support for 64bit and singed integers.

		int64_t absVal = 0;
		int len = str.size();
		int i=0;
		//Skip the minus
		if(str[0] == '-')
			i++;

		for(; i<len; i++)
		{	if(str[i] < '0' || str[i] > '9')
				return false;

			absVal *=10;
			absVal += str[i] - '0';
		}

		//negate the val if needed.
		*val = absVal * (str[0] == '-'?-1:1);
		return true;
	}

	static bool ToString(std::string *str, int64_t *val)
	{
		char buff[16];
		sprintf(buff, "%lld", *val);
		*str = buff;
		return true;
	}


	static bool TryParse(std::string str, uint64_t *val)
	{
		//Explicitly chosen to not use the KC_functions.
		//We need support for 64bit and singed integers.

		uint64_t absVal = 0;
		int len = str.size();
		int i=0;

		for(; i<len; i++)
		{
			if(str[i] < '0' || str[i] > '9')
				return false;

			absVal *=10;
			absVal += str[i] - '0';
		}

		*val = absVal;
		return true;
	}

	static bool ToString(std::string *str, uint64_t *val)
	{
		char buff[16];
		sprintf(buff, "%lld", *val);
		*str = buff;
		return true;
	}



	/*  ----------------------------  */
	/*	--  Floating point types  --  */
	/*  ----------------------------  */


	static bool TryParse(std::string str, float *val)
	{
		double i;
		bool success = TryParse(str, &i);
		if(success)
			*val = i;
		return success;
	}

	static bool ToString(std::string *str, float *val)
	{
		char buff[16];
		sprintf(buff, "%f", *val);
		*str = buff;
		return true;
	}



	static bool TryParse(std::string str, double *val)
	{

		double absVal = 0;
		int dotPos = -1;
		int len = str.size();
		int i=0;
		//Skip the minus
		if(str[0] == '-')
			i++;

		for(; i<len; i++)
		{
			if(str[i] == '.')
			{
				if(dotPos == -1)
					dotPos = i;
				else
					return false;
			}
			else if (str[i] >= '0' && str[i] <= '9')
			{
				absVal *=10;
				absVal += str[i] - '0';
			}
			else
			{
				return false;
			}
		}

		int div = 1;

		if(dotPos !=-1)		//divide the val if needed.
			div = pow(10, i-dotPos-1);

		if(str[0] == '-')	//negate the val if needed.
			div *= -1;

		absVal /= div;

		*val = absVal;
		return true;
	}

	static bool ToString(std::string *str, double *val)
	{
		char buff[16];
		sprintf(buff, "%lf", *val);
		*str = buff;
		return true;
	}


	/*  ----------------------------  */
	/*	----------  MISC  ----------  */
	/*  ----------------------------  */

	static bool TryParse(std::string str, Version *val)
	{
		uint8_t major, minor, beta;
		bool suc = true;

		suc &= TryParse(str.substr(0, 2), &major);
		suc &= TryParse(str.substr(3, 2), &minor);
		suc &= TryParse(str.substr(6, 2), &beta);

		*val = Version(major, minor, beta);

		return true;
	}



	static bool ToString(std::string *str, Version *val)
	{
		char buff[100];
		snprintf(buff, sizeof(buff), "%02d.%02d.%02d", val->GetMajor(), val->GetMinor(), val->GetBeta());
		*str = buff;
		return true;
	}

	static bool TryParse(std::string str, DateTime *val)
	{
		struct tm tm;
		bool suc = true;
		suc &= StringConverter::TryParse(str.substr(0, 2),  &tm.tm_mday);
		suc &= StringConverter::TryParse(str.substr(2, 2),  &tm.tm_mon);
		suc &= StringConverter::TryParse(str.substr(4, 4),  &tm.tm_year);
		suc &= StringConverter::TryParse(str.substr(8, 2),  &tm.tm_hour);
		suc &= StringConverter::TryParse(str.substr(10, 2), &tm.tm_min);
		suc &= StringConverter::TryParse(str.substr(12, 2), &tm.tm_sec);

		tm.tm_isdst = 0;
		tm.tm_mon-=1;
		tm.tm_year-=1900;

		if(suc)
			*val = DateTime::FromLocalTime(tm);
		return suc;
	}

	static bool ToString(std::string *str, DateTime *val)
	{
		char buff[32];
		struct tm tm = val->GetLocalTime();
		tm.tm_mon += 1;
		tm.tm_year += 1900;

		sprintf(buff, "%02d%02d%04d%02d%02d%02d",
				tm.tm_mday,
				tm.tm_mon,
				tm.tm_year,
				tm.tm_hour,
				tm.tm_min,
				tm.tm_sec
				);

		*str = std::string(buff);
		return true;
	}

};

