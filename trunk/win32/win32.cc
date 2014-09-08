#include "stdafx.h"

#include "sincity/sc_api.h"

#include <assert.h>

#define SC_LOCAL_IP			NULL // NULL means get the best
#define SC_LOCAL_PORT			0	// 0 means get the best
#define SC_REMOTE_REQUEST_URI	"ws://localhost:9000/wsStringStaticMulti?roomId=0"
#define SC_DEBUG_LEVEL			SCDebugLevel_Info
#define SC_SSL_CERT_PATH		"SSL_Public.pem"
#define SC_USERID_LOCAL			"001"
#define SC_USERID_REMOTE		"002"

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
	SC_ASSERT(SCEngine::setSSLCertificates(SC_SSL_CERT_PATH));

    SCObjWrapper<SCSignaling*>signalSession = SCSignaling::newObj(SC_REMOTE_REQUEST_URI, SC_LOCAL_IP, SC_LOCAL_PORT);
    SC_ASSERT(signalSession);

    SC_ASSERT(signalSession->connect());

    getchar();

	// SC_ASSERT(signalSession->sendData("salut", 5));
	SCObjWrapper<SCSessionCall*>callSession = SCSessionCall::newObj(SC_USERID_LOCAL, signalSession);
	SC_ASSERT(callSession);
	SC_ASSERT(callSession->call(SCMediaType_ScreenCast, SC_USERID_REMOTE));

	getchar();
	getchar();

    SC_ASSERT(signalSession->disConnect());

    return 0;
}


