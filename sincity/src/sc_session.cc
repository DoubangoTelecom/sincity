#include "sincity/sc_session.h"

SCSession::SCSession(SCSessionType_t eType)
    : m_eType(eType)
{

}

SCSession::~SCSession()
{
    stop();
}