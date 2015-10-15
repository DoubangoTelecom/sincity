#ifndef SINCITY_NETTRANSPORT_WS_H
#define SINCITY_NETTRANSPORT_WS_H

#include "sc_config.h"
#include "sincity/sc_url.h"
#include "sincity/sc_nettransport.h"

class SCWsTransport;

//
//	SCWsResult
//
class SCWsResult : public SCObj
{
public:
    SCWsResult(unsigned short nCode, const char* pcPhrase, const void* pcDataPtr = NULL, size_t nDataSize = 0, SCWsActionType_t eActionType = SCWsActionType_None);
    virtual ~SCWsResult();
    virtual SC_INLINE const char* getObjectId() {
        return "SCWsResult";
    }

    virtual SC_INLINE unsigned short getCode() {
        return m_nCode;
    }
    virtual SC_INLINE const char* getPhrase() {
        return m_pPhrase;
    }
    virtual SC_INLINE const void* getDataPtr() {
        return m_pDataPtr;
    }
    virtual SC_INLINE size_t getDataSize() {
        return m_nDataSize;
    }
    virtual SC_INLINE SCWsActionType_t getActionType() {
        return m_eActionType;
    }

private:
    unsigned short m_nCode;
    char* m_pPhrase;
    void* m_pDataPtr;
    size_t m_nDataSize;
    SCWsActionType_t m_eActionType;
};

//
//	SCWsTransportCallback
//
class SCWsTransportCallback : public SCNetTransportCallback
{
public:
    SCWsTransportCallback(const SCWsTransport* pcTransport);
    virtual ~SCWsTransportCallback();
    virtual SC_INLINE const char* getObjectId() {
        return "SCWsTransportCallback";
    }
    virtual bool onData(SCObjWrapper<SCNetPeer*> oPeer, size_t &nConsumedBytes);
    virtual bool onConnectionStateChanged(SCObjWrapper<SCNetPeer*> oPeer);
private:
    const SCWsTransport* m_pcTransport;
};


//
//	SCWsTransport
//
class SCWsTransport : public SCNetTransport
{
    friend class SCWsTransportCallback;
public:
    SCWsTransport(bool isSecure, const char* pcLocalIP = NULL, unsigned short nLocalPort = 0);
    virtual ~SCWsTransport();
    virtual SC_INLINE const char* getObjectId() {
        return "SCWsTransport";
    }

    bool handshaking(SCObjWrapper<SCNetPeer*> oPeer, SCObjWrapper<SCUrl*> oUrl);

private:
    SCObjWrapper<SCWsResult*> handleJsonContent(SCObjWrapper<SCNetPeer*> oPeer, const void* pcDataPtr, size_t nDataSize)const;
    bool sendResult(SCObjWrapper<SCNetPeer*> oPeer, SCObjWrapper<SCWsResult*> oResult)const;

private:
    SCObjWrapper<SCWsTransportCallback*> m_oCallback;
};


#endif /* SINCITY_NETTRANSPORT_WS_H */
