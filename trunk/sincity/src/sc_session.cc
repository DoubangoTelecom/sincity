#include "sincity/sc_session.h"

#include <assert.h>

SCSession::SCSession(SCSessionType_t eType, std::string strUserId, SCObjWrapper<SCSignaling*> oSignaling)
    : m_eType(eType)
	, m_oSignaling(oSignaling)
	, m_strUserId(strUserId)
{
	SC_ASSERT(m_oSignaling);
}

SCSession::~SCSession()
{
    
}