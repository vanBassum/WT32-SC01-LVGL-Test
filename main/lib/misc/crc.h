#pragma once

#include <stdint.h>


uint16_t CRC16_2(uint8_t* buf, int len)
{
	uint16_t crc = 0xFFFF;
	for (int pos = 0; pos < len; pos++)
	{
		crc ^= (uint16_t)buf[pos];    // XOR byte into least sig. byte of crc

		for(int i = 8 ; i != 0 ; i--)
		{
			if ((crc & 0x0001) != 0)
			{
				      // If the LSB is set
				crc >>= 1;                    // Shift right and XOR 0xA001
				crc ^= 0xA001;
			}
			else                            // Else LSB is not set
				crc >>= 1;                    // Just shift right
		}
	}
	return crc;
}

