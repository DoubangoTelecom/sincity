#include "sincity/sc_mutex.h"

SCMutex::SCMutex(bool bRecursive /*= true*/)
{
    m_phOTMutex = tsk_mutex_create_2(bRecursive ? tsk_true : tsk_false);
}

SCMutex::~SCMutex()
{
    if (m_phOTMutex) {
        tsk_mutex_destroy(&m_phOTMutex);
    }
    SC_DEBUG_INFO("*** SCMutex destroyed ***");
}

bool SCMutex::lock()
{
    return (tsk_mutex_lock(m_phOTMutex) == 0);
}

bool SCMutex::unlock()
{
    return (tsk_mutex_unlock(m_phOTMutex) == 0);
}
