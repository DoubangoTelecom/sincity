#ifndef SINCITY_ENGINE_H
#define SINCITY_ENGINE_H

#include "sc_config.h"
#include "sincity/sc_obj.h"
#include "sincity/sc_common.h"
#include "sincity/sc_session_call.h"
#include "sincity/sc_signaling.h"

#include <list>

class SCIceServer : public SCObj
{
	friend class SCEngine;
protected:
	SCIceServer(
		std::string strTransport,
		std::string strServerHost,
		unsigned short serverPort, 
		bool useTurn, 
		bool useStun, 
		std::string strUsername, 
		std::string strPassword)
	{
		m_strTransport = strTransport;
		m_strServerHost = strServerHost;
		m_uServerPort = serverPort; 
		m_bUseTurn = useTurn;
		m_bUseStun = useStun; 
		m_strUsername = strUsername;
		m_strPassword = strPassword;
	}
public:
	virtual ~SCIceServer() {}
	virtual SC_INLINE const char* getObjectId() { return "SCIceServer"; }
	SC_INLINE const char* getTransport()const { return m_strTransport.c_str(); }
	SC_INLINE const char* getServerHost()const { return m_strServerHost.c_str(); }
	SC_INLINE unsigned short getServerPort()const { return m_uServerPort; }
	SC_INLINE bool isTurnEnabled()const { return m_bUseTurn; }
	SC_INLINE bool isStunEnabled()const { return m_bUseStun; }
	SC_INLINE const char* getUsername()const { return m_strUsername.c_str(); }
	SC_INLINE const char* getPassword()const { return m_strPassword.c_str(); }

private:
	std::string m_strTransport;
	std::string m_strServerHost;
	unsigned short m_uServerPort; 
	bool m_bUseTurn;
	bool m_bUseStun; 
	std::string m_strUsername;
	std::string m_strPassword;
};

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
	static bool setVideoJbEnabled(bool enabled);
	static bool setVideoAvpfTail(int min, int max);
	static bool setVideoZeroArtifactsEnabled(bool enabled);
	static bool setAudioEchoSuppEnabled(bool enabled);
	static bool setAudioEchoTail(int tailLength);
	static bool SC_DEPRECATED(setNattStunServer)(const char* host, unsigned short port = 3478);
	static bool SC_DEPRECATED(setNattStunCredentials)(const char* username, const char* password);
	static bool addNattIceServer(const char* strTransportProto, const char* strServerHost, unsigned short serverPort, bool useTurn = false, bool useStun = true, const char* strUsername = NULL, const char* strPassword = NULL);
	static bool clearNattIceServers();
	static bool setNattIceStunEnabled(bool enabled);
	static bool setNattIceTurnEnabled(bool enabled);

private:
	static bool s_bInitialized;
	static std::string s_strCredUserId;
	static std::string s_strCredPassword;
	static std::list<SCObjWrapper<SCIceServer*> > s_listIceServers;
	static SCObjWrapper<SCMutex*> s_listIceServersMutex;
};

#endif /* SINCITY_ENGINE_H */
