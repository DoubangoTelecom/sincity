#ifndef SINCITY_ENGINE_H
#define SINCITY_ENGINE_H

#include "sc_config.h"
#include "sincity/sc_obj.h"
#include "sincity/sc_common.h"

class SCEngine : public SCObj
{
protected:
	SCEngine();
public:
	virtual ~SCEngine();
	virtual SC_INLINE const char* getObjectId() { return "SCEngine"; }

	static bool init();
	static bool deInit();
	static bool setDebugLevel(SCDebugLevel_t eLevel);
	static bool setSSLCertificates(const char* strPublicKey, const char* strPrivateKey = NULL, const char* strCA = NULL, bool bMutualAuth = false);

private:
	static bool s_bInitialized;
};

#endif /* SINCITY_ENGINE_H */
