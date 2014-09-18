#include "sincity/sc_signaling.h"
#include "sincity/sc_nettransport_ws.h"
#include "sincity/sc_engine.h"
#include "sincity/sc_parser_url.h"
#include "sincity/jsoncpp/sc_json.h"

#include "tinyhttp.h"
#include "tsk_string.h"

#include <assert.h>

#define SC_JSON_GET(fieldParent, fieldVarName, fieldName, typeTestFun, couldBeNull) \
	if(!fieldParent.isObject()) \
	{ \
		SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "JSON '%s' not an object", (fieldName)); \
		return false; \
	} \
	const Json::Value fieldVarName = (fieldParent)[(fieldName)]; \
	if((fieldVarName).isNull()) \
	{ \
		if(!(couldBeNull)) \
		{ \
			SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "JSON '%s' is null", (fieldName)); \
			return false; \
		}\
	} \
	if(!(fieldVarName).typeTestFun()) \
	{ \
		SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "JSON '%s' has invalid type", (fieldName)); \
		return false; \
	}

SCSignaling::SCSignaling(SCObjWrapper<SCNetTransport*>& oNetTransport, SCObjWrapper<SCUrl*>& oConnectionUrl)
    : m_oNetTransport(oNetTransport)
    , m_oConnectionUrl(oConnectionUrl)
    , m_Fd(kSCNetFdInvalid)
    , m_bWsHandshakingDone(false)
    , m_pWsSendBufPtr(NULL)
    , m_nWsSendBuffSize(0)
    , m_oMutex(new SCMutex())
{
    SC_ASSERT(m_oNetTransport);
    SC_ASSERT(m_oConnectionUrl);

    m_oNetCallback = new SCSignalingTransportCallback(this);
    m_oNetTransport->setCallback(*m_oNetCallback);
}

SCSignaling::~SCSignaling()
{
    TSK_FREE(m_pWsSendBufPtr);
}

void SCSignaling::lock()
{
    m_oMutex->lock();
}

void SCSignaling::unlock()
{
    m_oMutex->unlock();
}

bool SCSignaling::setCallback(SCObjWrapper<SCSignalingCallback*> oCallback)
{
    SCAutoLock<SCSignaling> autoLock(this);

    m_oSignCallback = oCallback;
    return true;
}

bool SCSignaling::isConnected()
{
    SCAutoLock<SCSignaling> autoLock(this);

    return m_oNetTransport->isConnected(m_Fd);
}

bool SCSignaling::isReady()
{
    SCAutoLock<SCSignaling> autoLock(this);

    if (!isConnected()) {
        return false;
    }
    if (m_oConnectionUrl->getType() == SCUrlType_WS || m_oConnectionUrl->getType() == SCUrlType_WSS) {
        return m_bWsHandshakingDone;
    }
    return true;
}

bool SCSignaling::connect()
{
    SCAutoLock<SCSignaling> autoLock(this);

    if (isConnected()) {
        SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "Already connected");
        return true;
    }

    if (!m_oNetTransport->isStarted()) {
        if (!m_oNetTransport->start()) {
            return false;
        }
    }

    SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "Connecting to [%s:%u]", m_oConnectionUrl->getHost().c_str(), m_oConnectionUrl->getPort());
    m_Fd = m_oNetTransport->connectTo(m_oConnectionUrl->getHost().c_str(), m_oConnectionUrl->getPort());
    return SCNetFd_IsValid(m_Fd);
}

bool SCSignaling::disConnect()
{
    SCAutoLock<SCSignaling> autoLock(this);

    if (isConnected()) {
        bool ret = m_oNetTransport->close(m_Fd);
        if (ret) {
            // m_Fd = kSCNetFdInvalid;
        }
        return ret;
    }
    return true;
}

bool SCSignaling::sendData(const void* _pcData, tsk_size_t _nDataSize)
{
    SCAutoLock<SCSignaling> autoLock(this);

    if (!_pcData || !_nDataSize) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Invalid argument");
        return false;
    }

    if (!isConnected()) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Not connected yet");
        return false;
    }

    if (m_oConnectionUrl->getType() == SCUrlType_WS || m_oConnectionUrl->getType() == SCUrlType_WSS) {
        if (!m_bWsHandshakingDone) {
            SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "WebSocket handshaking not done yet");
            return false;
        }
        uint8_t mask_key[4] = { 0x00, 0x00, 0x00, 0x00 };
        const uint8_t* pcData = (const uint8_t*)_pcData;
        uint64_t nDataSize = 1 + 1 + 4/*mask*/ + _nDataSize;
        uint64_t lsize = (uint64_t)_nDataSize;
        uint8_t* pws_snd_buffer;

        if (lsize > 0x7D && lsize <= 0xFFFF) {
            nDataSize += 2;
        }
        else if(lsize > 0xFFFF) {
            nDataSize += 8;
        }
        if (m_nWsSendBuffSize < nDataSize) {
            if (!(m_pWsSendBufPtr = tsk_realloc(m_pWsSendBufPtr, (tsk_size_t)nDataSize))) {
                SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Failed to allocate buffer with size = %llu", nDataSize);
                m_nWsSendBuffSize = 0;
                return 0;
            }
            m_nWsSendBuffSize = (tsk_size_t)nDataSize;
        }
        pws_snd_buffer = (uint8_t*)m_pWsSendBufPtr;

        pws_snd_buffer[0] = 0x81; // FIN | opcode-non-control::text
        pws_snd_buffer[1] = 0x80; // Set Mask flag (required for data from client->sever)

        if (lsize <= 0x7D) {
            pws_snd_buffer[1] |= (uint8_t)lsize;
            pws_snd_buffer = &pws_snd_buffer[2];
        }
        else if (lsize <= 0xFFFF) {
            pws_snd_buffer[1] |= 0x7E;
            pws_snd_buffer[2] = (lsize >> 8) & 0xFF;
            pws_snd_buffer[3] = (lsize & 0xFF);
            pws_snd_buffer = &pws_snd_buffer[4];
        }
        else {
            pws_snd_buffer[1] |= 0x7F;
            pws_snd_buffer[2] = (lsize >> 56) & 0xFF;
            pws_snd_buffer[3] = (lsize >> 48) & 0xFF;
            pws_snd_buffer[4] = (lsize >> 40) & 0xFF;
            pws_snd_buffer[5] = (lsize >> 32) & 0xFF;
            pws_snd_buffer[6] = (lsize >> 24) & 0xFF;
            pws_snd_buffer[7] = (lsize >> 16) & 0xFF;
            pws_snd_buffer[8] = (lsize >> 8) & 0xFF;
            pws_snd_buffer[9] = (lsize & 0xFF);
            pws_snd_buffer = &pws_snd_buffer[10];
        }

        // Mask Key
        pws_snd_buffer[0] = mask_key[0];
        pws_snd_buffer[1] = mask_key[1];
        pws_snd_buffer[2] = mask_key[2];
        pws_snd_buffer[3] = mask_key[3];
        pws_snd_buffer = &pws_snd_buffer[4];
        // Mask dat
        // ... nothing done because key is always zeros

        // append payload to headers
        memcpy(pws_snd_buffer, pcData, (size_t)lsize);
        // send() data
        return m_oNetTransport->sendData(m_Fd, m_pWsSendBufPtr, (tsk_size_t)nDataSize);
    }
    else {
        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Not implemented yet");
        return false;
    }
}

SCObjWrapper<SCSignaling*> SCSignaling::newObj(const char* pcRequestUri, const char* pcLocalIP /*= NULL*/, unsigned short nLocalPort /*= 0*/)
{
    SCObjWrapper<SCSignaling*> oSignaling;
    SCObjWrapper<SCUrl*> oUrl;
    SCObjWrapper<SCNetTransport*> oTransport;

    if (!SCEngine::isInitialized()) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Engine not initialized");
        goto bail;
    }

    if (tsk_strnullORempty(pcRequestUri)) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "RequestUri is null or empty");
        goto bail;
    }

    oUrl = sc_url_parse(pcRequestUri, tsk_strlen(pcRequestUri));
    if (!oUrl) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Failed to parse request Uri: %s", pcRequestUri);
        goto bail;
    }
    if (oUrl->getHostType() == SCUrlHostType_None) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Invalid host type: %s // %d", pcRequestUri, oUrl->getHostType());
        goto bail;
    }
    switch (oUrl->getType()) {
    case SCUrlType_WS:
    case SCUrlType_WSS: {
        oTransport = new SCWsTransport((oUrl->getType() == SCUrlType_WSS), pcLocalIP, nLocalPort);
        if (!oTransport) {
            goto bail;
        }
        break;
    }
    default: {
        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Url type=%d not supported yet. Url=%s", oUrl->getType(), pcRequestUri);
        goto bail;
    }
    }

    oSignaling = new SCSignaling(oTransport, oUrl);

bail:
    return oSignaling;
}

bool SCSignaling::handleData(const char* pcData, tsk_size_t nDataSize)
{
    SCAutoLock<SCSignaling> autoLock(this);

    Json::Value root;
    Json::Reader reader;

    // Parse JSON content
    bool parsingSuccessful = reader.parse((const char*)pcData, (((const char*)pcData) + nDataSize), root);
    if (!parsingSuccessful) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Failed to parse JSON content: %.*s", nDataSize, pcData);
        return false;
    }

    SC_JSON_GET(root, type, "type", isString, false);
    SC_JSON_GET(root, cid, "cid", isString, false);
    SC_JSON_GET(root, tid, "tid", isString, false);
    SC_JSON_GET(root, from, "from", isString, false);
    SC_JSON_GET(root, to, "to", isString, false);

    if (to.asString() != SCEngine::s_strCredUserId) {
		if (from.asString() == SCEngine::s_strCredUserId) {
			SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "Ignoring loopback message with type='%s', call-id='%s', to='%s'", type.asCString(), cid.asCString(), to.asCString());
		}
		else {
			SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "Failed to match destination id: '%s'<>'%s'", to.asCString(), SCEngine::s_strCredUserId.c_str());
		}
        return false;
    }
	
    bool bIsCallEventRequiringSdp = (type.asString().compare("offer") == 0 || type.asString().compare("answer") == 0 || type.asString().compare("pranswer") == 0);
    if (bIsCallEventRequiringSdp || type.asString().compare("hangup") == 0) {
        if (m_oSignCallback) {
            SCObjWrapper<SCSignalingCallEvent*> oCallEvent = new SCSignalingCallEvent(type.asString());
            if (bIsCallEventRequiringSdp) {
                SC_JSON_GET(root, sdp, "sdp", isString, false);
                oCallEvent->m_strSdp = sdp.asString();
            }
            oCallEvent->m_strType = type.asString();
            oCallEvent->m_strCallId = cid.asString();
            oCallEvent->m_strTransacId = tid.asString();
            oCallEvent->m_strFrom = from.asString();
            oCallEvent->m_strTo = to.asString();
            return m_oSignCallback->onEventCall(oCallEvent);
        }
    }
    else {
        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Message with type='%s' not supported yet", type.asString().c_str());
        return false;
    }

    return true;
}

bool SCSignaling::raiseEvent(SCSignalingEventType_t eType, std::string strDescription)
{
    SCAutoLock<SCSignaling> autoLock(this);

    if (m_oSignCallback) {
        SCObjWrapper<SCSignalingEvent*> e = new SCSignalingEvent(eType, strDescription);
        return m_oSignCallback->onEventNet(e);
    }
    return true;
}

//
//	SCSignalingTransportCallback
//

SCSignalingTransportCallback::SCSignalingTransportCallback(const SCSignaling* pcSCSignaling)
    : m_pcSCSignaling(pcSCSignaling)
{

}

SCSignalingTransportCallback::~SCSignalingTransportCallback()
{
    m_pcSCSignaling = NULL;
}

bool SCSignalingTransportCallback::onData(SCObjWrapper<SCNetPeer*> oPeer, size_t &nConsumedBytes)
{
    SCAutoLock<SCSignaling> autoLock(const_cast<SCSignaling*>(m_pcSCSignaling));

    SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "Incoming data = %.*s", oPeer->getDataSize(), (const char*)oPeer->getDataPtr());

    if (m_pcSCSignaling->m_oConnectionUrl->getType() != SCUrlType_WS && m_pcSCSignaling->m_oConnectionUrl->getType() != SCUrlType_WSS) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Not implemented yet");
        return false;
    }

    if (!m_pcSCSignaling->m_bWsHandshakingDone) {
        /* WebSocket handshaking data */

        if (oPeer->getDataSize() > 4) {
            const char* pcData = (const char*)oPeer->getDataPtr();
            if (pcData[0] == 'H' && pcData[1] == 'T' && pcData[2] == 'T' && pcData[3] == 'P') {
                int endOfMessage = tsk_strindexOf(pcData, oPeer->getDataSize(), "\r\n\r\n"/*2CRLF*/) + 4;
                if (endOfMessage > 4) {
                    thttp_message_t *p_msg = tsk_null;
                    const thttp_header_Sec_WebSocket_Accept_t* http_hdr_accept;
                    tsk_ragel_state_t ragel_state;

                    tsk_ragel_state_init(&ragel_state, pcData, (tsk_size_t)endOfMessage);
                    if (thttp_message_parse(&ragel_state, &p_msg, tsk_false) != 0) {
                        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Failed to parse HTTP message: %.*s", endOfMessage, pcData);
                        TSK_OBJECT_SAFE_FREE(p_msg);
                        return const_cast<SCSignaling*>(m_pcSCSignaling)->raiseEvent(SCSignalingEventType_NetError, "WebSocket handshaking: Failed to parse HTTP message");
                    }
                    if (!THTTP_MESSAGE_IS_RESPONSE(p_msg)) {
                        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Incoming HTTP message not a response: %.*s", endOfMessage, pcData);
                        TSK_OBJECT_SAFE_FREE(p_msg);
                        return const_cast<SCSignaling*>(m_pcSCSignaling)->raiseEvent(SCSignalingEventType_NetError, "WebSocket handshaking: Incoming HTTP message not a response");
                    }
                    if (p_msg->line.response.status_code > 299) {
                        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Incoming HTTP response is an error: %.*s", endOfMessage, pcData);
                        TSK_OBJECT_SAFE_FREE(p_msg);
                        return const_cast<SCSignaling*>(m_pcSCSignaling)->raiseEvent(SCSignalingEventType_NetError, "WebSocket handshaking: Incoming HTTP response is an error");
                    }
                    // Get Accept header
                    if (!(http_hdr_accept = (const thttp_header_Sec_WebSocket_Accept_t*)thttp_message_get_header(p_msg, thttp_htype_Sec_WebSocket_Accept))) {
                        SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "No 'Sec-WebSocket-Accept' header: %.*s", endOfMessage, pcData);
                        TSK_OBJECT_SAFE_FREE(p_msg);
                        return const_cast<SCSignaling*>(m_pcSCSignaling)->raiseEvent(SCSignalingEventType_NetError, "WebSocket handshaking: No 'Sec-WebSocket-Accept' header");
                    }
                    // Authenticate the response
                    {
                        thttp_auth_ws_keystring_t resp = {0};
                        thttp_auth_ws_response(oPeer->getWsKey(), &resp);
                        if (!tsk_striequals(http_hdr_accept->value, resp)) {
                            SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Authentication failed: %.*s", endOfMessage, pcData);
                            TSK_OBJECT_SAFE_FREE(p_msg);
                            return const_cast<SCSignaling*>(m_pcSCSignaling)->raiseEvent(SCSignalingEventType_NetError, "WebSocket handshaking: Authentication failed");
                        }
                    }
                    TSK_OBJECT_SAFE_FREE(p_msg);

                    const_cast<SCSignaling*>(m_pcSCSignaling)->m_bWsHandshakingDone = true;
                    nConsumedBytes = endOfMessage;
                    return const_cast<SCSignaling*>(m_pcSCSignaling)->raiseEvent(SCSignalingEventType_NetReady, "WebSocket handshaking: done");
                }
            }
        }
    }
    else {
        /* WebSocket raw data */
        const uint8_t* pcData = (const uint8_t*)oPeer->getDataPtr();
        const uint8_t opcode = pcData[0] & 0x0F;
        if ((pcData[0] & 0x01)/* FIN */) {
            const uint8_t mask_flag = (pcData[1] >> 7); // Must be "1" for "client -> server"
            uint8_t mask_key[4] = { 0x00 };
            uint64_t pay_idx;
            uint64_t data_len = 0;
            uint64_t pay_len = 0;

            if (pcData[0] & 0x40 || pcData[0] & 0x20 || pcData[0] & 0x10) {
                SC_DEBUG_ERROR_EX(kSCMobuleNameSignaling, "Unknown extension: %d", (pcData[0] >> 4) & 0x07);
                return const_cast<SCSignaling*>(m_pcSCSignaling)->raiseEvent(SCSignalingEventType_NetReady, "WebSocket data: Unknown extension");
            }

            pay_len = pcData[1] & 0x7F;
            data_len = 2;

            if (pay_len == 126) {
                if (oPeer->getDataSize() < 4) {
                    SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "No all data in the WS buffer");
                    nConsumedBytes = 0;
                    return true;
                }
                pay_len = (pcData[2] << 8 | pcData[3]);
                pcData = &pcData[4];
                data_len += 2;
            }
            else if (pay_len == 127) {
                if ((oPeer->getDataSize() - data_len) < 8) {
                    SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "No all data in the WS buffer");
                    nConsumedBytes = 0;
                    return true;
                }
                pay_len = (((uint64_t)pcData[2]) << 56 | ((uint64_t)pcData[3]) << 48 | ((uint64_t)pcData[4]) << 40 | ((uint64_t)pcData[5]) << 32 | ((uint64_t)pcData[6]) << 24 | ((uint64_t)pcData[7]) << 16 | ((uint64_t)pcData[8]) << 8 || ((uint64_t)pcData[9]));
                pcData = &pcData[10];
                data_len += 8;
            }
            else {
                pcData = &pcData[2];
            }

            if (mask_flag) {
                // must be "true"
                if ((oPeer->getDataSize() - data_len) < 4) {
                    SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "No all data in the WS buffer");
                    nConsumedBytes = 0;
                    return true;
                }
                mask_key[0] = pcData[0];
                mask_key[1] = pcData[1];
                mask_key[2] = pcData[2];
                mask_key[3] = pcData[3];
                pcData = &pcData[4];
                data_len += 4;
            }

            if ((oPeer->getDataSize() - data_len) < pay_len) {
                SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "No all data in the WS buffer");
                nConsumedBytes = 0;
                return true;
            }

            data_len += pay_len;
            uint8_t* _pcData = const_cast<uint8_t*>(pcData);

            // unmasking the payload
            if (mask_flag) {
                for (pay_idx = 0; pay_idx < pay_len; ++pay_idx) {
                    _pcData[pay_idx] = (pcData[pay_idx] ^ mask_key[(pay_idx & 3)]);
                }
            }
            return const_cast<SCSignaling*>(m_pcSCSignaling)->handleData((const char*)_pcData, (tsk_size_t)pay_len);
        }
        else if (opcode == 0x08) {
            // RFC6455 - 5.5.1.  Close
            SC_DEBUG_INFO_EX(kSCMobuleNameSignaling, "WebSocket opcode 0x8 (Close)");
            const_cast<SCSignaling*>(m_pcSCSignaling)->m_oNetTransport->close(oPeer->getFd());
        }
    }
    return true;
}

bool SCSignalingTransportCallback::onConnectionStateChanged(SCObjWrapper<SCNetPeer*> oPeer)
{
    SCAutoLock<SCSignaling> autoLock(const_cast<SCSignaling*>(m_pcSCSignaling));

    if ((oPeer->getFd() == m_pcSCSignaling->m_Fd || m_pcSCSignaling->m_Fd == kSCNetFdInvalid) && oPeer->isConnected()) {
        const SCWsTransport* pcTransport = dynamic_cast<const SCWsTransport*>(*m_pcSCSignaling->m_oNetTransport);
        SC_ASSERT(pcTransport);
        const_cast<SCSignaling*>(m_pcSCSignaling)->raiseEvent(SCSignalingEventType_NetConnected, "Connected");
        if ((m_pcSCSignaling->m_oConnectionUrl->getType() == SCUrlType_WS || m_pcSCSignaling->m_oConnectionUrl->getType() == SCUrlType_WSS)) {
            if (!m_pcSCSignaling->m_bWsHandshakingDone) {
                return const_cast<SCWsTransport*>(pcTransport)->handshaking(oPeer, const_cast<SCSignaling*>(m_pcSCSignaling)->m_oConnectionUrl);
            }
        }
        return const_cast<SCSignaling*>(m_pcSCSignaling)->raiseEvent(SCSignalingEventType_NetReady, "Ready");
    }
    else if (m_pcSCSignaling->m_Fd == oPeer->getFd() && !oPeer->isConnected()) {
        const_cast<SCSignaling*>(m_pcSCSignaling)->m_Fd = kSCNetFdInvalid;
        const_cast<SCSignaling*>(m_pcSCSignaling)->m_bWsHandshakingDone = false;
        return const_cast<SCSignaling*>(m_pcSCSignaling)->raiseEvent(SCSignalingEventType_NetDisconnected, "Disconnected");
    }

    return true;
}

//
//	SCCallEvent
//

SCSignalingCallEvent::SCSignalingCallEvent(std::string strDescription)
    : SCSignalingEvent(SCSignalingEventType_Call, strDescription)
{

}

SCSignalingCallEvent::~SCSignalingCallEvent()
{

}
