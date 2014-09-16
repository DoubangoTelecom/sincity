#include "sincity/sc_nettransport.h"

#include "tsk_base64.h"

#include <assert.h>

#if !defined(kSCMaxStreamBufferSize)
#	define kSCMaxStreamBufferSize 0xFFFF
#endif

//
//	SCNetPeer
//

#if 0
// IMPORTANT: data sent using this function will never be encrypted
bool SCNetPeer::sendData(const void* pcDataPtr, size_t nDataSize)
{
    if (!pcDataPtr || !nDataSize) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameNetTransport, "Invalid parameter");
        return false;
    }
    return (tnet_sockfd_send(getFd(), pcDataPtr, nDataSize, 0) == nDataSize);
}
#endif

bool SCNetPeer::buildWsKey()
{
    char WsKey[30];
    const int count = sizeof(WsKey)/sizeof(WsKey[0]);
    for (int i = 0; i < count - 1; ++i) {
        WsKey[i] = rand() % 0xFF;
    }
    WsKey[count - 1] = '\0';

    return tsk_base64_encode((const uint8_t*)WsKey, (count - 1), &m_pWsKey) > 0;
}

//
//	SCNetTransport
//
SCNetTransport::SCNetTransport(SCNetTransporType_t eType, const char* pcLocalIP, unsigned short nLocalPort)
    : m_bValid(false)
    , m_bStarted(false)
{
    m_eType = eType;
    const char *pcDescription;
    tnet_socket_type_t eSocketType;
    bool bIsIPv6 = false;

    if(pcLocalIP && nLocalPort) {
        bIsIPv6 = (tnet_get_family(pcLocalIP, nLocalPort) == AF_INET6);
    }

    switch (eType) {
    case SCNetTransporType_TCP:
    case SCNetTransporType_WS: {
        pcDescription = bIsIPv6 ? "TCP/IPv6 transport" : "TCP/IPv4 transport";
        eSocketType = bIsIPv6 ? tnet_socket_type_tcp_ipv6 : tnet_socket_type_tcp_ipv4;
        break;
    }
    case SCNetTransporType_TLS:
    case SCNetTransporType_WSS: {
        pcDescription = bIsIPv6 ? "TLS/IPv6 transport" : "TLS/IPv4 transport";
        eSocketType = bIsIPv6 ? tnet_socket_type_tls_ipv6 : tnet_socket_type_tls_ipv4;
        break;
    }
    default: {
        SC_ASSERT(false);
        return;
    }
    }

    if ((m_pWrappedTransport = tnet_transport_create(pcLocalIP, nLocalPort, eSocketType, pcDescription))) {
        if (TNET_SOCKET_TYPE_IS_STREAM(eSocketType)) {
            tnet_transport_set_callback(m_pWrappedTransport, SCNetTransport::SCNetTransportCb_Stream, this);
        }
        else {
            SC_ASSERT(false);
            return;
        }
    }

    m_oPeersMutex = new SCMutex();

    m_bValid = (m_oPeersMutex && m_pWrappedTransport);
}

SCNetTransport::~SCNetTransport()
{
    stop();
    TSK_OBJECT_SAFE_FREE(m_pWrappedTransport);
    SC_DEBUG_INFO("*** SCNetTransport destroyed ***");
}

bool SCNetTransport::setSSLCertificates(const char* pcPrivateKey, const char* pcPublicKey, const char* pcCA, bool bVerify /*= false*/)
{
    return (tnet_transport_tls_set_certs(m_pWrappedTransport, pcCA, pcPublicKey, pcPrivateKey, (bVerify ? tsk_true : tsk_false)) == 0);
}

bool SCNetTransport::start()
{
    m_bStarted = (tnet_transport_start(m_pWrappedTransport) == 0);
    return m_bStarted;
}

SCNetFd SCNetTransport::connectTo(const char* pcHost, unsigned short nPort)
{
    if (!pcHost || !nPort) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameNetTransport, "Invalid parameter");
        return TNET_INVALID_FD;
    }
    if (!isValid()) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameNetTransport, "Transport not valid");
        return TNET_INVALID_FD;
    }
    if(!isStarted()) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameNetTransport, "Transport not started");
        return TNET_INVALID_FD;
    }

    return tnet_transport_connectto_2(m_pWrappedTransport, pcHost, nPort);
}

bool SCNetTransport::isConnected(SCNetFd nFd)
{
    SCObjWrapper<SCNetPeer*> oPeer = getPeerByFd(nFd);
    return (oPeer && oPeer->isConnected());
}

bool SCNetTransport::sendData(SCNetFd nFdFrom, const void* pcDataPtr, size_t nDataSize)
{
    if (!pcDataPtr || !nDataSize || !SCNetFd_IsValid(nFdFrom)) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameNetTransport, "Invalid parameter");
        return false;
    }
    return (tnet_transport_send(m_pWrappedTransport, nFdFrom, pcDataPtr, nDataSize) == nDataSize);
}

bool SCNetTransport::sendData(SCObjWrapper<SCNetPeer*> oPeer, const void* pcDataPtr, size_t nDataSize)
{
    return sendData(oPeer->getFd(), pcDataPtr, nDataSize);
}

bool SCNetTransport::close(SCNetFd nFd)
{
    return (tnet_transport_remove_socket(m_pWrappedTransport, &nFd) == 0);
}

bool SCNetTransport::stop()
{
    m_bStarted = false;
    return (tnet_transport_shutdown(m_pWrappedTransport) == 0);
}

SCObjWrapper<SCNetPeer*> SCNetTransport::getPeerByFd(SCNetFd nFd)
{
    m_oPeersMutex->lock();
    SCObjWrapper<SCNetPeer*> m_Peer = NULL;

    std::map<SCNetFd, SCObjWrapper<SCNetPeer*> >::iterator iter = m_Peers.find(nFd);
    if (iter != m_Peers.end()) {
        m_Peer = iter->second;
    }
    m_oPeersMutex->unlock();

    return m_Peer;
}

void SCNetTransport::insertPeer(SCObjWrapper<SCNetPeer*> oPeer)
{
    m_oPeersMutex->lock();
    if (oPeer) {
        m_Peers.insert( std::pair<SCNetFd, SCObjWrapper<SCNetPeer*> >(oPeer->getFd(), oPeer) );
    }
    m_oPeersMutex->unlock();
}

void SCNetTransport::removePeer(SCNetFd nFd)
{
    m_oPeersMutex->lock();
    std::map<SCNetFd, SCObjWrapper<SCNetPeer*> >::iterator iter;
    if ((iter = m_Peers.find(nFd)) != m_Peers.end()) {
        SCObjWrapper<SCNetPeer*> oPeer = iter->second;
        m_Peers.erase(iter);
    }
    m_oPeersMutex->unlock();
}

int SCNetTransport::SCNetTransportCb_Stream(const tnet_transport_event_t* e)
{
    SCObjWrapper<SCNetPeer*> oPeer = NULL;
    SCNetTransport* This = (SCNetTransport*)e->callback_data;

    switch (e->type) {
    case event_removed:
    case event_closed: {
        oPeer = (This)->getPeerByFd(e->local_fd);
        if (oPeer) {
            oPeer->setConnected(false);
            (This)->removePeer(e->local_fd);
            if ((This)->m_oCallback) {
                (This)->m_oCallback->onConnectionStateChanged(oPeer);
            }
        }
        break;
    }
    case event_connected:
    case event_accepted: {
        oPeer = (This)->getPeerByFd(e->local_fd);
        if (oPeer) {
            oPeer->setConnected(true);
        }
        else {
            oPeer = new SCNetPeerStream(e->local_fd, true);
            (This)->insertPeer(oPeer);
        }
        if ((This)->m_oCallback) {
            (This)->m_oCallback->onConnectionStateChanged(oPeer);
        }
        break;
    }

    case event_data: {
        oPeer = (This)->getPeerByFd(e->local_fd);
        if (!oPeer) {
            SC_DEBUG_ERROR_EX(kSCMobuleNameNetTransport, "Data event but no peer found!");
            return -1;
        }

        size_t nConsumedBytes = oPeer->getDataSize();
        if ((nConsumedBytes + e->size) > kSCMaxStreamBufferSize) {
            SC_DEBUG_ERROR_EX(kSCMobuleNameNetTransport, "Stream buffer too large[%u > %u]. Did you forget to consume the bytes?", (nConsumedBytes + e->size), kSCMaxStreamBufferSize);
            dynamic_cast<SCNetPeerStream*>(*oPeer)->cleanupData();
        }
        else {
            if ((This)->m_oCallback) {
                if (dynamic_cast<SCNetPeerStream*>(*oPeer)->appenData(e->data, e->size)) {
                    nConsumedBytes += e->size;
                }
                (This)->m_oCallback->onData(oPeer, nConsumedBytes);
            }
            if (nConsumedBytes) {
                dynamic_cast<SCNetPeerStream*>(*oPeer)->remoteData(0, nConsumedBytes);
            }
        }
        break;
    }

    case event_error:
    default: {
        break;
    }
    }

    return 0;
}