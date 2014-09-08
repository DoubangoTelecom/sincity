#ifndef SINCITY_SIGNALING_H
#define SINCITY_SIGNALING_H

#include "sc_config.h"
#include "sincity/sc_obj.h"
#include "sincity/sc_nettransport.h"
#include "sincity/sc_url.h"
#include "sincity/sc_mutex.h"

class SCSignaling;

class SCSignalingEvent : public SCObj
{
public:
	SCSignalingEvent(SCSignalingEventType_t eType, std::string strDescription) : m_eType(eType), m_strDescription(strDescription) {}
	virtual ~SCSignalingEvent() {}
	virtual SC_INLINE const char* getObjectId() { return "SCSignalingEvent"; }

	virtual SC_INLINE SCSignalingEventType_t getType()const { return m_eType; }
	virtual SC_INLINE std::string getDescription()const { return m_strDescription; }

private:
	SCSignalingEventType_t m_eType;
	std::string m_strDescription;
};

class SCSignalingTransportCallback : public SCNetTransportCallback
{
public:
	SCSignalingTransportCallback(const SCSignaling* pcSCSignaling);
	virtual ~SCSignalingTransportCallback();
	virtual SC_INLINE const char* getObjectId() { return "SCSignalingTransportCallback"; }

	virtual bool onData(SCObjWrapper<SCNetPeer*> oPeer, size_t &nConsumedBytes);

	virtual bool onConnectionStateChanged(SCObjWrapper<SCNetPeer*> oPeer);

private:
	const SCSignaling* m_pcSCSignaling;
};

class SCSignaling : public SCObj
{
	friend class SCSignalingTransportCallback;
	friend class SCAutoLock<SCSignaling>;
protected:
	SCSignaling(SCObjWrapper<SCNetTransport*>& oNetTransport, SCObjWrapper<SCUrl*>& oConnectionUrl);
public:
	virtual ~SCSignaling();
	virtual SC_INLINE const char* getObjectId() { return "SCSignaling"; }

	bool isConnected();
	bool isReady();
	bool connect();
	bool disConnect();
	bool sendData(const void* pcData, tsk_size_t nDataSize);

	static SCObjWrapper<SCSignaling*> newObj(const char* pcRequestUri, const char* pcLocalIP = NULL, unsigned short nLocalPort = 0);

private:
	bool handleData(const char* pcData, tsk_size_t nDataSize);
	bool raiseEvent(SCSignalingEventType_t eType, std::string strDescription);

	void lock();
	void unlock();

private:
	SCObjWrapper<SCNetTransport*> m_oNetTransport;
	SCObjWrapper<SCUrl*> m_oConnectionUrl;
	SCObjWrapper<SCSignalingTransportCallback*> m_oNetCallback;
	SCNetFd m_Fd;
	bool m_bWsHandshakingDone;
	void* m_pWsSendBufPtr;
	tsk_size_t m_nWsSendBuffSize;
	SCObjWrapper<SCMutex*> m_oMutex;
};

#endif /* SINCITY_SIGNALING_H */
