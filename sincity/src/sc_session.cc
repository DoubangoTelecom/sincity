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

/**@ingroup _Group_CPP_Session
 * Sends data to the server.
 * @param _pcData Pointer to the data to send.
 * @param _nDataSize Size (in bytes) of the data to send.
 * @retval <b>true</b> if no error; otherwise <b>false</b>.
 */
bool SCSession::sendData(const void* pcData, size_t nDataSize)
{
    return m_oSignaling->sendData(pcData, nDataSize);
}
