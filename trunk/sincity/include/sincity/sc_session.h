#ifndef SINCITY_SESSION_H
#define SINCITY_SESSION_H

#include "sc_config.h"
#include "sincity/sc_common.h"
#include "sincity/sc_obj.h"

class SCSession : public SCObj
{
protected:
	SCSession(SCSessionType_t eType);
public:
	virtual ~SCSession();
	virtual SC_INLINE const char* getObjectId() { return "SCSession"; }

	virtual bool start() = 0;
	virtual bool stop() = 0;

	virtual SC_INLINE SCSessionType_t getType()const { return m_eType; }

private:
	SCSessionType_t m_eType;
};

#endif /* SINCITY_SESSION_H */
