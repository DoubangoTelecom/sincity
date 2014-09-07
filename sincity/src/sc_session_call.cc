#include "sincity/sc_session_call.h"

SCSessionCall::SCSessionCall()
    : SCSession(SCSessionType_Call)
    , m_bStarted(false)
{

}

SCSessionCall::~SCSessionCall()
{

}

bool SCSessionCall::start()
{
    return false;
}

bool SCSessionCall::stop()
{
    return false;
}

SCObjWrapper<SCSessionCall*> SCSessionCall::newObj()
{
    SCObjWrapper<SCSessionCall*> oCall = new SCSessionCall();

    return oCall;
}
