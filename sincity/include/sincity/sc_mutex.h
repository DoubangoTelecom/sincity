#ifndef SINCITY_MUTEX_H
#define SINCITY_MUTEX_H

#include "sc_config.h"
#include "sincity/sc_common.h"
#include "sincity/sc_obj.h"

class SCMutex : public SCObj
{
public:
    SCMutex(bool bRecursive = true);
    virtual ~SCMutex();
    virtual SC_INLINE const char* getObjectId() {
        return "SCMutex";
    }
    bool lock();
    bool unlock();

private:
    SCNativeMutexHandle_t* m_phOTMutex;
};

#endif /* SINCITY_MUTEX_H */
