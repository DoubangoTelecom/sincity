#ifndef SINCITY_SESSION_CALL_H
#define SINCITY_SESSION_CALL_H

#include "sc_config.h"
#include "sincity/sc_session.h"
#include "sincity/sc_signaling.h"

#include <string>

class SCSessionCall : public SCSession
{
protected:
	SCSessionCall();
public:
	
	virtual ~SCSessionCall();
	virtual SC_INLINE const char* getObjectId() { return "SCSessionCall"; }

	virtual bool start();
	virtual bool stop();

	static SCObjWrapper<SCSessionCall*> newObj();

private:
	bool m_bStarted;
};


#endif /* SINCITY_SESSION_CALL_H */
