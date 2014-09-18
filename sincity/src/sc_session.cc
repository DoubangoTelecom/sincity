#include "sincity/sc_session.h"

#include <assert.h>

/**@defgroup _Group_CPP_Session Base session
* @brief Base session class.
*/

SCSession::SCSession(SCSessionType_t eType, SCObjWrapper<SCSignaling*> oSignaling)
    : m_eType(eType)
    , m_oSignaling(oSignaling)
{
    SC_ASSERT(m_oSignaling);
}

SCSession::~SCSession()
{

}