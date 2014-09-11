#include "stdafx.h"

#include "sincity/sc_api.h"

#include <assert.h>

#define SC_LOCAL_IP			NULL // NULL means get the best
#define SC_LOCAL_PORT			0	// 0 means get the best
#define SC_REMOTE_REQUEST_URI	"ws://192.168.0.37:9000/wsStringStaticMulti?roomId=0"
#define SC_DEBUG_LEVEL			SCDebugLevel_Info
#define SC_SSL_PATH_PUB			"SSL_Pub.pem"
#define SC_SSL_PATH_PRIV		"SSL_Priv.pem"
#define SC_SSL_PATH_CA			"SSL_CA.pem"
#define SC_USERID_LOCAL			"001"
#define SC_USERID_REMOTE		"002"

#define SC_DEMO_AS_CLIENT			0

SCObjWrapper<SCSessionCall*>callSession;
SCObjWrapper<SCSignaling*>signalSession;

class SCSignalingCallbackDummy : public SCSignalingCallback
{
protected:
    SCSignalingCallbackDummy() {

    }
public:
    virtual ~SCSignalingCallbackDummy() {

    }

    virtual bool onEventNet(SCObjWrapper<SCSignalingEvent*>& e) {
        //!\Deadlock issue: You must not call any function from 'SCSignaling' class unless you fork a new thread.
        switch (e->getType()) {
        case SCSignalingEventType_NetReady: {
#if SC_DEMO_AS_CLIENT
            if (!callSession) {
                callSession = SCSessionCall::newObj(signalSession);
                SC_ASSERT(callSession);
                SC_ASSERT(callSession->call(SCMediaType_ScreenCast, SC_USERID_REMOTE));
            }
#endif
            break;
        }
        }

        return true;
    }
    virtual SC_INLINE const char* getObjectId() {
        return "SCSignalingCallbackDummy";
    }
    virtual bool onEventCall(SCObjWrapper<SCSignalingCallEvent*>& e) {
        //!\Deadlock issue: You must not call any function from 'SCSignaling' class unless you fork a new thread.
        if (callSession) {
            if (callSession->getCallId() != e->getCallId()) {
                SC_DEBUG_ERROR("Call id mismatch: '%s'<>'%s'", callSession->getCallId().c_str(), e->getCallId().c_str());
                return e->reject();
            }
            bool ret = callSession->handEvent(e);
			if (e->getType() == "hangup") {
				callSession = NULL;
			}
			return ret;
        }
        else {
            if (e->getType() == "offer") {
#if SC_DEMO_AS_CLIENT
                return e->reject();
#else
				SC_ASSERT(callSession = SCSessionCall::newObj(signalSession, e));
				SC_ASSERT(callSession->handEvent(e));
#endif
            }
            // Silently ignore any other event type
        }

        return true;
    }

    static SCObjWrapper<SCSignalingCallback*> newObj() {
        return new SCSignalingCallbackDummy();
    }
};

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
    SC_ASSERT(SCEngine::init(SC_USERID_LOCAL));
	SC_ASSERT(SCEngine::setSSLCertificates(SC_SSL_PATH_PUB, SC_SSL_PATH_PRIV, SC_SSL_PATH_CA));

    signalSession = SCSignaling::newObj(SC_REMOTE_REQUEST_URI, SC_LOCAL_IP, SC_LOCAL_PORT);
    SC_ASSERT(signalSession);

    SC_ASSERT(signalSession->setCallback(SCSignalingCallbackDummy::newObj()));

    SC_ASSERT(signalSession->connect());

#if SC_UNDER_WINDOWS && 0
	MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#else 
	getchar();
#endif

    return 0;
}


