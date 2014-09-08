#ifndef SINCITY_ENGINE_H
#define SINCITY_ENGINE_H

#include "sc_config.h"
#include "sincity/sc_obj.h"
#include "sincity/sc_common.h"
#include "sincity/sc_session_call.h"
#include "sincity/sc_signaling.h"

class SCEngine : public SCObj
{
	friend class SCSessionCall;
	friend class SCSignaling;
protected:
	SCEngine();
public:
	virtual ~SCEngine();
	virtual SC_INLINE const char* getObjectId() { return "SCEngine"; }

	static bool init(std::string strCredUserId, std::string strCredPassword = "");
	static bool deInit();
	static bool isInitialized() { return s_bInitialized; }
	static bool setDebugLevel(SCDebugLevel_t eLevel);
	static bool setSSLCertificates(const char* strPublicKey, const char* strPrivateKey = NULL, const char* strCA = NULL, bool bMutualAuth = false);

private:
	static bool s_bInitialized;
	static std::string s_strCredUserId;
	static std::string s_strCredPassword;
	static std::string s_strStunServerAddr;
	static unsigned short s_nStunServerPort;
	static std::string s_strStunUsername;
	static std::string s_strStunPassword;
};

#endif /* SINCITY_ENGINE_H */
