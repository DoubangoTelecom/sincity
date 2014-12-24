#include "sincity/sc_thread.h"

#include "tsk_thread.h"

#include <assert.h>

SCThread::SCThread(SCNativeThreadHandle_t* phThread)
    : m_phThread(phThread)
{
    SC_ASSERT(phThread);
}

SCThread::~SCThread()
{
    join();
    SC_DEBUG_INFO("*** SCThread destroyed ***");
}

bool SCThread::join()
{
    bool bRet = false;
    if (m_phThread) {
        SC_DEBUG_INFO("SCThread join ---ENTER---");
        bRet = (tsk_thread_join(&m_phThread) == 0);
        SC_DEBUG_INFO("SCThread join ---EXIT---");
    }
    return bRet;
}

SCObjWrapper<SCThread*> SCThread::newObj(void *(SC_STDCALL *start) (void *), void *arg /*= NULL*/)
{
    SCNativeThreadHandle_t *phThread = tsk_null;
    if (tsk_thread_create(&phThread, start, arg) != 0) {
        return NULL;
    }
    return new SCThread(phThread);
}
