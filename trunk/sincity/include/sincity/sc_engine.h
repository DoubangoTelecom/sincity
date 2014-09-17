#ifndef SINCITY_ENGINE_H
#define SINCITY_ENGINE_H

#include "sc_config.h"
#include "sincity/sc_obj.h"
#include "sincity/sc_common.h"
#include "sincity/sc_session_call.h"
#include "sincity/sc_signaling.h"

/**
* Static class used to configure the media engine.
*/
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
	static bool setSSLCertificates(const char* strPublicKey, const char* strPrivateKey, const char* strCA, bool bMutualAuth = false);
	static bool setVideoPrefSize(const char* strPrefVideoSize);
	static bool setVideoFps(int fps);
	static bool setVideoBandwidthUpMax(int bandwwidthMax);
	static bool setVideoBandwidthDownMax(int bandwwidthMax);
	static bool setVideoMotionRank(int motionRank);
	static bool setVideoCongestionCtrlEnabled(bool congestionCtrl);
	static bool setNattStunServer(const char* host, unsigned short port = 3478);
	static bool setNattStunCredentials(const char* username, const char* password);
	static bool setNattIceStunEnabled(bool enabled);
	static bool setNattIceTurnEnabled(bool enabled);

private:
	static bool s_bInitialized;
	static std::string s_strCredUserId;
	static std::string s_strCredPassword;
};

#endif /* SINCITY_ENGINE_H */
