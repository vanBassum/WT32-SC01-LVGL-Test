/*
 * Connection.h
 *
 *  Created on: 23 Oct 2020
 *      Author: Bas
 */

#ifndef MAIN_CPP_LIB_JBVPROTOCOL_CONNECTION_H_
#define MAIN_CPP_LIB_JBVPROTOCOL_CONNECTION_H_

#include "stdint.h"
#include "../Misc/Callback.h"



class IConnection
{

public:
	virtual ~IConnection(){}
	Callback<void, IConnection*, uint8_t*, size_t> OnDataReceived;
	Callback<void, IConnection*> OnDispose;
	virtual void Send(uint8_t* data, size_t length) = 0;
	

};


#endif /* MAIN_CPP_LIB_JBVPROTOCOL_CONNECTION_H_ */
