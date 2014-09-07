#ifndef SINCITY_MUTEX_H
#define SINCITY_MUTEX_H

#include "sc_config.h"
#include "sincity/sc_obj.h"

#include "tsk_mutex.h"

class SCMutex : public SCObj
{	
public:
	SCMutex(bool bRecursive = true);
	virtual ~SCMutex();
	virtual SC_INLINE const char* getObjectId() { return "SCMutex"; }
	bool lock();
	bool unlock();

private:
	tsk_mutex_handle_t* m_phOTMutex;
};

#endif /* SINCITY_MUTEX_H */
