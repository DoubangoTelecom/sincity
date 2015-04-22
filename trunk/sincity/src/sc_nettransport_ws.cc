#include "sincity/sc_nettransport_ws.h"
#include "sincity/sc_engine.h"
#include "sincity/jsoncpp/sc_json.h"

#include "tsk_string.h"
#include "tsk_memory.h"

#include "tinyhttp.h"
#include <assert.h>

/* min size of a stream chunck to form a valid HTTP message */
#define kStreamChunckMinSize 0x32
#define kHttpMethodOptions "OPTIONS"
#define kHttpMethodPost "POST"

#define kSCWsResultCode_Provisional		100
#define kSCWsResultCode_Success			200
#define kSCWsResultCode_Unauthorized		403
#define kSCWsResultCode_NotFound			404
#define kSCWsResultCode_ParsingFailed		420
#define kSCWsResultCode_InvalidDataType	483
#define kSCWsResultCode_InvalidData		450
#define kSCWsResultCode_InternalError		603

#define kSCWsResultPhrase_Success					"OK"
#define kSCWsResultPhrase_Unauthorized			"Unauthorized"
#define kSCWsResultPhrase_NotFound				"Not Found"
#define kSCWsResultPhrase_ParsingFailed			"Parsing failed"
#define kSCWsResultPhrase_InvalidDataType			"Invalid data type"
#define kSCWsResultPhrase_FailedToCreateLocalFile	"Failed to create local file"
#define kSCWsResultPhrase_FailedTransferPending	"Failed transfer pending"
#define kSCWsResultPhrase_InternalError			"Internal Error"

#define kJsonField_Action "action"
#define kJsonField_Name "name"
#define kJsonField_Type "type"



static SC_INLINE bool isJsonContentType(const thttp_message_t *pcHttpMessage);
static const char* getHttpContentType(const thttp_message_t *pcHttpMessage);

#define SC_JSON_GET(fieldParent, fieldVarName, fieldName, typeTestFun, couldBeNull) \
	if(!fieldParent.isObject()){ \
		SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "JSON '%s' not an object", (fieldName)); \
		return new SCWsResult(kSCWsResultCode_InvalidDataType, kSCWsResultPhrase_InvalidDataType); \
	} \
	const Json::Value fieldVarName = (fieldParent)[(fieldName)]; \
	if((fieldVarName).isNull()) \
	{ \
		if(!(couldBeNull)){ \
				SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "JSON '%s' is null", (fieldName)); \
				return new SCWsResult(kSCWsResultCode_InvalidDataType, "Required field is missing"); \
		}\
	} \
	if(!(fieldVarName).typeTestFun()) \
	{ \
		SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "JSON '%s' has invalid type", (fieldName)); \
		return new SCWsResult(kSCWsResultCode_InvalidDataType, kSCWsResultPhrase_InvalidDataType); \
	}

//
//	SCWsResult
//

SCWsResult::SCWsResult(unsigned short nCode, const char* pcPhrase, const void* pcDataPtr, size_t nDataSize, SCWsActionType_t eActionType)
    : m_pDataPtr(NULL)
    , m_nDataSize(0)
    , m_eActionType(eActionType)
{
    m_nCode = nCode;
    m_pPhrase = tsk_strdup(pcPhrase);
    if(pcDataPtr && nDataSize) {
        if((m_pDataPtr = tsk_malloc(nDataSize))) {
            memcpy(m_pDataPtr, pcDataPtr, nDataSize);
            m_nDataSize = nDataSize;
        }
    }
}

SCWsResult::~SCWsResult()
{
    TSK_FREE(m_pPhrase);
    TSK_FREE(m_pDataPtr);

    SC_DEBUG_INFO("*** SCWsResult destroyed ***");
}

//
//	SCWsTransport
//

SCWsTransport::SCWsTransport(bool isSecure, const char* pcLocalIP /*= NULL*/, unsigned short nLocalPort /*= 0*/)
    : SCNetTransport(isSecure ? SCNetTransporType_WSS : SCNetTransporType_WS, pcLocalIP, nLocalPort)
{
    m_oCallback = new SCWsTransportCallback(this);
    setCallback(*m_oCallback);
}

SCWsTransport::~SCWsTransport()
{
    setCallback(NULL);

    SC_DEBUG_INFO("*** SCWsTransport destroyed ***");
}

bool SCWsTransport::handshaking(SCObjWrapper<SCNetPeer*> oPeer, SCObjWrapper<SCUrl*> oUrl)
{
    if (!isConnected(oPeer->getFd())) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Peer with fd=%d not connected yet", oPeer->getFd());
        return false;
    }
    if (!oPeer->buildWsKey()) {
        return false;
    }

    tnet_ip_t localIP;
    tnet_port_t local_port;
    if (tnet_transport_get_ip_n_port(m_pWrappedTransport, oPeer->getFd(), &localIP, &local_port) != 0) {
        return false;
    }

    char* requestUri = tsk_null;
    if (oUrl->getHPath().empty()) {
        requestUri = tsk_strdup("/");
    }
    else {
        tsk_sprintf(&requestUri, "/%s", oUrl->getHPath().c_str());
        if (!oUrl->getSearch().empty()) {
            tsk_strcat_2(&requestUri, "?%s", oUrl->getSearch().c_str());
        }
    }

#define WS_REQUEST_GET_FORMAT "GET %s HTTP/1.1\r\n" \
	   "Host: %s\r\n" \
	   "Upgrade: websocket\r\n" \
	   "Connection: Upgrade\r\n" \
	   "Sec-WebSocket-Key: %s\r\n" \
	   "Origin: %s\r\n" \
	   "Sec-WebSocket-Protocol: ge-webrtc-signaling\r\n" \
	   "Sec-WebSocket-Version: 13\r\n" \
		"\r\n"

    char* request = tsk_null;
    tsk_sprintf(&request, WS_REQUEST_GET_FORMAT,
                requestUri,
                oUrl->getHost().c_str(),
                oPeer->getWsKey(),
                localIP);

    TSK_FREE(requestUri);

    bool ret = sendData(oPeer, request, tsk_strlen(request));
    TSK_FREE(request);
    return ret;
}

SCObjWrapper<SCWsResult*> SCWsTransport::handleJsonContent(SCObjWrapper<SCNetPeer*> oPeer, const void* pcDataPtr, size_t nDataSize)const
{
    SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Not implemented");
    return NULL;
}

bool SCWsTransport::sendResult(SCObjWrapper<SCNetPeer*> oPeer, SCObjWrapper<SCWsResult*> oResult)const
{
    SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Not implemented");
    return false;
}

//
//	SCWsTransportCallback
//
SCWsTransportCallback::SCWsTransportCallback(const SCWsTransport* pcTransport)
{
    m_pcTransport = pcTransport;
}

SCWsTransportCallback::~SCWsTransportCallback()
{

}

bool SCWsTransportCallback::onData(SCObjWrapper<SCNetPeer*> oPeer, size_t &nConsumedBytes)
{
    SC_DEBUG_INFO_EX(kSCMobuleNameWsTransport, "Incoming data = %.*s", (int)oPeer->getDataSize(), (const char*)oPeer->getDataPtr());
    SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Not implemented");
    return false;
}

bool SCWsTransportCallback::onConnectionStateChanged(SCObjWrapper<SCNetPeer*> oPeer)
{
    if (!oPeer->isConnected()) {

    }
    return true;
}

static const char* getHttpContentType(const thttp_message_t *pcHttpMessage)
{
    const thttp_header_Content_Type_t* contentType;

    if(!pcHttpMessage) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Invalid parameter");
        return NULL;
    }

    if ((contentType = (const thttp_header_Content_Type_t*)thttp_message_get_header(pcHttpMessage, thttp_htype_Content_Type))) {
        return contentType->type;
    }
    return NULL;
}

static SC_INLINE bool isContentType(const thttp_message_t *pcHttpMessage, const char* pcContentTypeToCompare)
{
    // content-type without parameters
    const char* pcContentType = getHttpContentType(pcHttpMessage);
    if (pcContentType) {
        return tsk_striequals(pcContentTypeToCompare, pcContentType);
    }
    return false;
}

#if 0
static SC_INLINE bool isJsonContentType(const thttp_message_t *pcHttpMessage)
{
    return isContentType(pcHttpMessage, kJsonContentType);
}
#endif
