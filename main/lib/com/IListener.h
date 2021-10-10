#pragma once


#include "../Misc/Callback.h"
#include "IConnection.h"
#include "../Misc/IJSON.h"
#include "../protocol/discovery.h"



class IListener
{
public:
	Callback<void, IListener*, IConnection*> OnConnectionAccepted;
	virtual JBV::ListenerInfo *GetListenerInfo() = 0;
};

