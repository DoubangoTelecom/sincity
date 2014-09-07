#include "stdafx.h"

#include "sincity/sc_api.h"

#include <assert.h>

#define SC_LOCAL_IP			NULL // NULL means get the best
#define SC_LOCAL_PORT			0	// 0 means get the best
#define SC_REMOTE_REQUEST_URI	"ws://localhost:9000/wsStringStaticMulti?roomId=0"
#define SC_DEBUG_LEVEL			SCDebugLevel_Info

int _tmain(int argc, _TCHAR* argv[])
{
    printf("*******************************************************************\n"
           "Copyright (C) 2014 Doubango Telecom (VoIP division)\n"
           "PRODUCT: SINCITY\n"
           "HOME PAGE: <void>\n"
           "CODE SOURCE: <void>\n"
           "LICENCE: <void>\n"
           "VERSION: %s\n"
           "'quit' to quit the application.\n"
           "*******************************************************************\n\n"
           , SC_VERSION_STRING);

    SCEngine::setDebugLevel(SC_DEBUG_LEVEL);
    SC_ASSERT(SCEngine::init());

    SCObjWrapper<SCSignaling*>signalSession = SCSignaling::newObj(SC_REMOTE_REQUEST_URI, SC_LOCAL_IP, SC_LOCAL_PORT);
    SC_ASSERT(signalSession);

    SC_ASSERT(signalSession->connect());

    getchar();

	SC_ASSERT(signalSession->sendData("salut", 5));

	getchar();

    SC_ASSERT(signalSession->disConnect());

    return 0;
}


