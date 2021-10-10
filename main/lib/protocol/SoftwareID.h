/*
 * SoftwareID.h
 *
 *  Created on: 23 Oct 2020
 *      Author: Bas
 */

#ifndef MAIN_CPP_LIB_JBVPROTOCOL_SOFTWAREID_H_
#define MAIN_CPP_LIB_JBVPROTOCOL_SOFTWAREID_H_


enum class SoftwareID
{
    Unknown = 0,
    //Router = 1,
    LeaseServer = 2,
    TileMapClient = 3,
    TileMapServer = 4,
    DPS50xx = 5,
    TestApp = 6,    //This id can be used when testing stuff. Its a device that doens't really exists.
    ConnectionServer = 7,
    DebugTool = 8,
	FunctionGenerator = 9,
};


#endif /* MAIN_CPP_LIB_JBVPROTOCOL_SOFTWAREID_H_ */
