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
	friend class SCSignaling;
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

class SCSignalingCallEvent : public SCSignalingEvent
{
	friend class SCSignaling;
public:
	SCSignalingCallEvent(std::string strDescription);
	virtual ~SCSignalingCallEvent();
	virtual SC_INLINE const char* getObjectId() { return "SCSignalingCallEvent"; }

	SC_INLINE std::string getType() { return m_strType; }
	SC_INLINE std::string getFrom() { return m_strFrom; }
	SC_INLINE std::string getTo() { return m_strTo; }
	SC_INLINE std::string getCallId() { return m_strCallId; }
	SC_INLINE std::string getTransacId() { return m_strTransacId; }
	SC_INLINE std::string getSdp() { return m_strSdp; }

private:
	std::string m_strFrom;
	std::string m_strTo;
	std::string m_strSdp;
	std::string m_strType;
	std::string m_strCallId;
	std::string m_strTransacId;
};

/**@ingroup _Group_CPP_Signaling
* Callback class for the signaling session. You must override this call.
*/
class SCSignalingCallback : public SCObj
{
protected:
	SCSignalingCallback() {}
public:
	virtual ~SCSignalingCallback() {}
	/** Raised to signal events releated to the network connection states */
	virtual bool onEventNet(SCObjWrapper<SCSignalingEvent*>& e) = 0;
	/** Raised to signal events related to the call states */
	virtual bool onEventCall(SCObjWrapper<SCSignalingCallEvent*>& e) = 0;
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

	bool setCallback(SCObjWrapper<SCSignalingCallback*> oCallback);
	bool isConnected();
	bool isReady();
	bool connect();
	bool sendData(const void* pcData, tsk_size_t nDataSize);
	bool disConnect();

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
	SCObjWrapper<SCSignalingCallback*> m_oSignCallback;
	SCNetFd m_Fd;
	bool m_bWsHandshakingDone;
	void* m_pWsSendBufPtr;
	tsk_size_t m_nWsSendBuffSize;
	SCObjWrapper<SCMutex*> m_oMutex;
};

#endif /* SINCITY_SIGNALING_H */
