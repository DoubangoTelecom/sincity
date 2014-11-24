#ifndef SINCITY_THREAD_H
#define SINCITY_THREAD_H

#include "sc_config.h"
#include "sincity/sc_common.h"
#include "sincity/sc_obj.h"

class SCThread : public SCObj
{	
protected:
	SCThread(SCNativeThreadHandle_t* phThread);
public:
	virtual ~SCThread();
	virtual SC_INLINE const char* getObjectId() { return "SCThread"; }
	bool join();
	static SCObjWrapper<SCThread*> newObj(void *(SC_STDCALL *start) (void *), void *arg = NULL);

private:
	SCNativeThreadHandle_t* m_phThread;
};

#endif /* SINCITY_THREAD_H */
