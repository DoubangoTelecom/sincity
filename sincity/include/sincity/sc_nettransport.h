#ifndef SINCITY_NETTRANSPORT_H
#define SINCITY_NETTRANSPORT_H

#include "sc_config.h"
#include "sincity/sc_common.h"
#include "sincity/sc_obj.h"
#include "sincity/sc_mutex.h"

#include <map>
#include <string>

class SCNetTransport;
class SCNetTransportCallback;

//
//	SCNetPeer
//
class SCNetPeer : public SCObj
{
    friend class SCNetTransport;
    friend class SCNetTransportCallback;
public:
    SCNetPeer(SCNetFd nFd, bool bConnected = false, const void* pcData = NULL, size_t nDataSize = 0);
    virtual ~SCNetPeer();
    virtual SC_INLINE SCNetFd getFd() {
        return  m_nFd;
    }
    virtual SC_INLINE bool isConnected() {
        return  m_bConnected;
    }
    virtual const void* getDataPtr();
    virtual size_t getDataSize();
    virtual SC_INLINE bool isRawContent() {
        return  m_bRawContent;
    }
    virtual SC_INLINE void setRawContent(bool bRawContent) {
        m_bRawContent = bRawContent;
    }
    virtual bool buildWsKey();
    virtual const char* getWsKey() {
        return m_pWsKey;
    }
    virtual SC_INLINE bool isStream() = 0;

protected:
    virtual SC_INLINE void setConnected(bool bConnected) {
        m_bConnected = bConnected;
    }

protected:
    bool m_bConnected;
    bool m_bRawContent;
    SCNetFd m_nFd;
    struct tsk_buffer_s* m_pWrappedBuffer;
    char* m_pWsKey;
};


//
//	SCNetPeerDgram
//
class SCNetPeerDgram : public SCNetPeer
{
public:
    SCNetPeerDgram(SCNetFd nFd, const void* pcData = NULL, size_t nDataSize = 0)
        :SCNetPeer(nFd, false, pcData, nDataSize) {
    }
    virtual ~SCNetPeerDgram() {
    }
    virtual SC_INLINE const char* getObjectId() {
        return "SCNetPeerDgram";
    }
    virtual SC_INLINE bool isStream() {
        return false;
    }
};

//
//	SCNetPeerStream
//
class SCNetPeerStream : public SCNetPeer
{
public:
    SCNetPeerStream(SCNetFd nFd, bool bConnected = false, const void* pcData = NULL, size_t nDataSize = 0)
        :SCNetPeer(nFd, bConnected, pcData, nDataSize) {
    }
    virtual ~SCNetPeerStream() {
    }
    virtual SC_INLINE const char* getObjectId() {
        return "SCNetPeerStream";
    }
    virtual SC_INLINE bool isStream() {
        return true;
    }
    virtual bool appenData(const void* pcData, size_t nDataSize);
    virtual bool remoteData(size_t nPosition, size_t nSize);
    virtual bool cleanupData();
};

//
//	SCNetTransport
//
class SCNetTransportCallback : public SCObj
{
public:
    SCNetTransportCallback() {
    }
    virtual ~SCNetTransportCallback() {
    }
    virtual bool onData(SCObjWrapper<SCNetPeer*> oPeer, size_t &nConsumedBytes) = 0;
    virtual bool onConnectionStateChanged(SCObjWrapper<SCNetPeer*> oPeer) = 0;
};

//
//	SCNetTransport
//
class SCNetTransport : public SCObj
{
protected:
    SCNetTransport(SCNetTransporType_t eType, const char* pcLocalIP, unsigned short nLocalPort);
public:
    virtual ~SCNetTransport();
    virtual SC_INLINE SCNetTransporType_t getType() {
        return m_eType;
    }
    virtual SC_INLINE bool isStarted() {
        return m_bStarted;
    }
    virtual SC_INLINE bool isValid() {
        return m_bValid;
    }
    virtual SC_INLINE void setCallback(SCObjWrapper<SCNetTransportCallback*> oCallback) {
        m_oCallback = oCallback;
    }

    virtual bool setSSLCertificates(const char* pcPrivateKey, const char* pcPublicKey, const char* pcCA, bool bVerify = false);
    virtual bool start();
    virtual SCNetFd connectTo(const char* pcHost, unsigned short nPort);
    virtual bool isConnected(SCNetFd nFd);
    virtual bool sendData(SCNetFd nFdFrom, const void* pcDataPtr, size_t nDataSize);
    virtual bool sendData(SCObjWrapper<SCNetPeer*> oPeer, const void* pcDataPtr, size_t nDataSize);
    virtual bool close(SCNetFd nFd);
    virtual bool stop();

private:
    bool hasPeer(SCNetFd nFd);
    SCObjWrapper<SCNetPeer*> getPeerByFd(SCNetFd nFd);
    void insertPeer(SCObjWrapper<SCNetPeer*> oPeer);
    void removePeer(SCNetFd nFd);
    static int SCNetTransportCb_Stream(const struct tnet_transport_event_s* e);

protected:
    SCNativeNetTransportHandle_t* m_pWrappedTransport;
    SCNetTransporType_t m_eType;
    bool m_bValid, m_bStarted;
    std::map<SCNetFd, SCObjWrapper<SCNetPeer*> > m_Peers;
    SCObjWrapper<SCNetTransportCallback*> m_oCallback;
    SCObjWrapper<SCMutex*> m_oPeersMutex;
};


#endif /* SINCITY_NETTRANSPORT_H */
