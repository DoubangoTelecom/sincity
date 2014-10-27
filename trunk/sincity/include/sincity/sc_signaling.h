#ifndef SINCITY_SIGNALING_H
#define SINCITY_SIGNALING_H

#include "sc_config.h"
#include "sincity/sc_obj.h"
#include "sincity/sc_nettransport.h"
#include "sincity/sc_url.h"
#include "sincity/sc_mutex.h"

class SCSignaling;

/**@ingroup _Group_CPP_Signaling
* Signaling event.
*/
class SCSignalingEvent : public SCObj
{
	friend class SCSignaling;
public:
	SCSignalingEvent(SCSignalingEventType_t eType, std::string strDescription, const void* pcDataPtr = NULL, size_t nDataSize = 0);
	virtual ~SCSignalingEvent();
	virtual SC_INLINE const char* getObjectId() { return "SCSignalingEvent"; }

	/**< The event type */
	virtual SC_INLINE SCSignalingEventType_t getType()const { return m_eType; }
	/**< The event description */
	virtual SC_INLINE std::string getDescription()const { return m_strDescription; }
	/**< The event data pointer */
	virtual SC_INLINE const void* getDataPtr()const { return m_pDataPtr; }
	virtual SC_INLINE size_t getDataSize()const { return m_nDataSize; }

private:
	SCSignalingEventType_t m_eType;
	std::string m_strDescription;
	void* m_pDataPtr;
	size_t m_nDataSize;
};


/**@ingroup _Group_CPP_Signaling
* Signaling event for call sessions.
*/
class SCSignalingCallEvent : public SCSignalingEvent
{
	friend class SCSignaling;
public:
	SCSignalingCallEvent(std::string strDescription);
	virtual ~SCSignalingCallEvent();
	virtual SC_INLINE const char* getObjectId() { return "SCSignalingCallEvent"; }

	/**< The event type. e.g. "offer", "answer", "hangup"... */
	SC_INLINE std::string getType() { return m_strType; }
	/**< The source identifier */ 
	SC_INLINE std::string getFrom() { return m_strFrom; }
	/**< The destination identifier */
	SC_INLINE std::string getTo() { return m_strTo; }
	/**< The call identifier */
	SC_INLINE std::string getCallId() { return m_strCallId; }
	/**< The transaction identifier */
	SC_INLINE std::string getTransacId() { return m_strTransacId; }
	/**< The session description. Could be NULL. */
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

	static SCObjWrapper<SCSignaling*> newObj(const char* pcConnectionUri, const char* pcLocalIP = NULL, unsigned short nLocalPort = 0);

private:
	bool handleData(const char* pcData, tsk_size_t nDataSize);
	bool raiseEvent(SCSignalingEventType_t eType, std::string strDescription, const void* pcDataPtr = NULL, size_t nDataSize = 0);

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
