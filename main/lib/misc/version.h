/*
 * version.h
 *
 *  Created on: Apr 14, 2021
 *      Author: Bas
 */

#ifndef MAIN_ESPLIB_MISC_VERSION_H_
#define MAIN_ESPLIB_MISC_VERSION_H_

#include <stdint.h>
#include <string.h>
#include <string>
#include <cJSON.h>
#include "IJSON.h"

class Version
{
	//Don't change the size, if you do check all items that inherit JSONSerializable
	uint8_t _version[3] = {0, 0, 0};

public:

	Version()
	{
	}

	Version(uint8_t version[3])
	{
		memcpy(_version, version, 3);
	}

	Version(uint8_t v1, uint8_t v2, uint8_t v3)
	{
		_version[0] = v1;
		_version[1] = v2;
		_version[2] = v3;
	}


	std::string ToString()
	{
		char buff[100];
		snprintf(buff, sizeof(buff), "%02d.%02d.%02d", _version[0], _version[1], _version[2]);
		std::string buffAsStdStr = buff;
		return buffAsStdStr;
	}



	bool operator == (Version v){ return memcmp(_version, v._version, 3) == 0; }
	bool operator >  (Version v){ return memcmp(_version, v._version, 3) >  0; }
	bool operator <  (Version v){ return memcmp(_version, v._version, 3) <  0; }
	bool operator >= (Version v){ return memcmp(_version, v._version, 3) >= 0; }
	bool operator <= (Version v){ return memcmp(_version, v._version, 3) <= 0; }

};
#endif /* MAIN_ESPLIB_MISC_VERSION_H_ */
