#pragma once

#include <stddef.h>
#include <stdint.h>
#include "../misc/callback.h"


class Stream
{
public:
	Callback<void, Stream*, uint8_t*, size_t> DataReceived;
	virtual void Write(uint8_t* data, size_t size) = 0;
};
