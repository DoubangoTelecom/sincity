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
    if(!pcDataPtr || !nDataSize) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Invalid parameter");
        return new SCWsResult(kSCWsResultCode_InternalError, "Invalid parameter");
    }

    Json::Value root;
    Json::Reader reader;

    // Parse JSON content
    bool parsingSuccessful = reader.parse((const char*)pcDataPtr, (((const char*)pcDataPtr) + nDataSize), root);
    if (!parsingSuccessful) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Failed to parse JSON content: %.*s", nDataSize, pcDataPtr);
        return new SCWsResult(kSCWsResultCode_ParsingFailed, kSCWsResultPhrase_ParsingFailed);
    }

    // JSON::action
    SC_JSON_GET(root, action, kJsonField_Action, isString, false);

    /* JSON::action=='FIXME' */
    if (tsk_striequals(action.asString().c_str(), "kJsonValue_ActionReq_UploadPresentation")) { // FIXME
#if 0
        // JSON::name
        SC_JSON_GET(root, name, kJsonField_Name, isString, false);
        // JSON::type
        SC_JSON_GET(root, type, kJsonField_Type, isString, false);
        // JSON::size
        SC_JSON_GET(root, size, kJsonField_Size, isIntegral, true);
        // JSON::bridge_id
        SC_JSON_GET(root, bridge_id, kJsonField_BridgeId, isString, false);
        // JSON::bridge_pin
        SC_JSON_GET(root, bridge_pin, kJsonField_BridgePin, isString, true);
        // JSON::user_id
        SC_JSON_GET(root, user_id, kJsonField_UserId, isString, false);

        // FIXME: make sure the bridge exist and is running (hard-coded bridges should allow uploading files)

        // create the file info
        SCObjWrapper<SCNetFileUpload* > oFileInfo = new SCNetFileUpload(
            oPeer->getFd(),
            name.asString(),
            type.asString(),
            size.asUInt(),
            bridge_id.asString(),
            bridge_pin.asString(),
            user_id.asString());

        if(!oFileInfo->isValid()) {
            return new SCWsResult(kSCWsResultCode_InternalError, kSCWsResultPhrase_FailedToCreateLocalFile);
        }
        if(m_oFileUploads.find(oFileInfo->getNetFd()) != m_oFileUploads.end()) {
            return new SCWsResult(kSCWsResultCode_InternalError, kSCWsResultPhrase_FailedTransferPending);
        }
        // store the file info
        const_cast<SCWsTransport*>(this)->m_oFileUploads[oFileInfo->getNetFd()] = oFileInfo;
        return new SCWsResult(kSCWsResultCode_Success, "File created", NULL, 0, SCWsActionType_UploadPresensation);
#endif
    }

    SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "'%s' not valid JSON action", action.asString().c_str());
    return new SCWsResult(kSCWsResultCode_InvalidDataType, "Invalid action type");
}

bool SCWsTransport::sendResult(SCObjWrapper<SCNetPeer*> oPeer, SCObjWrapper<SCWsResult*> oResult)const
{
    // FIXME
    return false;
#if 0
    if (!oResult || !oPeer) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Invalid parameter");
        return new SCWsResult(kSCWsResultCode_InternalError, "Invalid parameter");
    }

    bool bRet = false;
    void* pResult = NULL;

    int len = tsk_sprintf(
                  (char**)&pResult,
                  "HTTP/1.1 %u %s\r\n"
                  "Server: TelePresence Server " SC_VERSION_STRING "\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "Content-Length: %u\r\n"
                  "Content-Type: " kJsonContentType "\r\n"
                  "Connection: %s\r\n"
                  "\r\n",
                  oResult->getCode(),
                  oResult->getPhrase(),
                  oResult->getDataSize(),
                  (oResult->getActionType() == SCWsActionType_UploadPresensation) ? "keep-alive" : "Close"
              );

    if(len <= 0 || !pResult) {
        goto bail;
    }
    if(oResult->getDataPtr() && oResult->getDataSize()) {
        if(!(pResult = tsk_realloc(pResult, (len + oResult->getDataSize())))) {
            goto bail;
        }
        memcpy(&((uint8_t*)pResult)[len], oResult->getDataPtr(), oResult->getDataSize());
        len += oResult->getDataSize();
    }

    // send data
    bRet = const_cast<SCWsTransport*>(this)->sendData(oPeer->getFd(), pResult, len);

bail:
    TSK_FREE(pResult);
    return bRet;
#endif
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
    SC_DEBUG_INFO_EX(kSCMobuleNameWsTransport, "Incoming data = %.*s", oPeer->getDataSize(), oPeer->getDataPtr());
    return false; // FIXME
#if 0
    size_t nDataSize;
    const int8_t* pcDataPtr;
    int32_t endOfheaders;
    bool haveAllContent = false;
    thttp_message_t *httpMessage = tsk_null;
    tsk_ragel_state_t ragelState;
    static const tsk_bool_t bExtractContentFalse = tsk_false;
    static const size_t kHttpOptionsResponseSize = tsk_strlen(kHttpOptionsResponse);
    int ret;

#if 0
    SC_DEBUG_INFO_EX(kSCMobuleNameWsTransport, "Incoming data = %.*s", oPeer->getDataSize(), oPeer->getDataPtr());
#endif

    nConsumedBytes = 0;

    // https://developer.mozilla.org/en-US/docs/HTTP/Access_control_CORS


    /* Check if we have all HTTP headers. */
parse_buffer:
    pcDataPtr = ((const int8_t*)oPeer->getDataPtr()) + nConsumedBytes;
    nDataSize = (oPeer->getDataSize() - nConsumedBytes);

    if (oPeer->isRawContent()) {
        endOfheaders = 0;
        goto parse_rawcontent;
    }
    if ((endOfheaders = tsk_strindexOf((const char*)pcDataPtr, nDataSize, "\r\n\r\n"/*2CRLF*/)) < 0) {
        SC_DEBUG_INFO_EX(kSCMobuleNameWsTransport, "No all HTTP headers in the TCP buffer");
        goto bail;
    }

    /* If we are here this mean that we have all HTTP headers.
    *	==> Parse the HTTP message without the content.
    */
    tsk_ragel_state_init(&ragelState, (const char*)pcDataPtr, endOfheaders + 4/*2CRLF*/);
    if((ret = thttp_message_parse(&ragelState, &httpMessage, bExtractContentFalse)) == 0) {
        const thttp_header_Transfer_Encoding_t* transfer_Encoding;

        /* chunked? */
        if((transfer_Encoding = (const thttp_header_Transfer_Encoding_t*)thttp_message_get_header(httpMessage, thttp_htype_Transfer_Encoding)) && tsk_striequals(transfer_Encoding->encoding, "chunked")) {
            const char* start = (const char*)(pcDataPtr + (endOfheaders + 4/*2CRLF*/));
            const char* end = (const char*)(pcDataPtr + nDataSize);
            int index;

            SC_DEBUG_INFO_EX(kSCMobuleNameWsTransport, "HTTP CHUNKED transfer");

            while(start < end) {
                /* RFC 2616 - 19.4.6 Introduction of Transfer-Encoding */
                // read chunk-size, chunk-extension (if any) and CRLF
                tsk_size_t chunk_size = (tsk_size_t)tsk_atox(start);
                if((index = tsk_strindexOf(start, (end-start), "\r\n")) >=0) {
                    start += index + 2/*CRLF*/;
                }
                else {
                    SC_DEBUG_INFO_EX(kSCMobuleNameWsTransport, "Parsing chunked data has failed.");
                    break;
                }

                if(chunk_size == 0 && ((start + 2) <= end) && *start == '\r' && *(start+ 1) == '\n') {
                    int parsed_len = (start - (const char*)(pcDataPtr)) + 2/*CRLF*/;
#if 0
                    tsk_buffer_remove(dialog->buf, 0, parsed_len);
#else
                    nConsumedBytes +=
#endif
                    haveAllContent = true;
                    break;
                }

                thttp_message_append_content(httpMessage, start, chunk_size);
                start += chunk_size + 2/*CRLF*/;
            }
        }
        else {
            tsk_size_t clen = THTTP_MESSAGE_CONTENT_LENGTH(httpMessage); /* MUST have content-length header. */
            if(clen == 0) {
                /* No content */
                nConsumedBytes += (endOfheaders + 4/*2CRLF*/);/* Remove HTTP headers and CRLF ==> must never happen */
                haveAllContent = true;
            }
            else {
                /* There is a content */
                if(isFileContentType(httpMessage)) { // Is it for file-upload?
                    oPeer->setRawContent(true);
                    endOfheaders += 4/*2CRLF*/;
                    goto parse_rawcontent;
                }

                if((endOfheaders + 4/*2CRLF*/ + clen) > nDataSize) {
                    /* There is content but not all the content. */
                    SC_DEBUG_INFO_EX(kSCMobuleNameWsTransport, "No all HTTP content in the TCP buffer.");
                    goto bail;
                }
                else {
                    /* Add the content to the message. */
                    thttp_message_add_content(httpMessage, tsk_null, pcDataPtr + endOfheaders + 4/*2CRLF*/, clen);
                    /* Remove HTTP headers, CRLF and the content. */
                    nConsumedBytes += (endOfheaders + 4/*2CRLF*/ + clen);
                    haveAllContent = true;
                }
            }
        }
    }
    else {
        // fails to parse an HTTP message with all headers
        nConsumedBytes += endOfheaders + 4/*2CRLF*/;
    }


    if(httpMessage && haveAllContent) {
        /* Analyze HTTP message */
        if(THTTP_MESSAGE_IS_REQUEST(httpMessage)) {
            /* OPTIONS */
            if(tsk_striequals(httpMessage->line.request.method, kHttpMethodOptions)) {
                // https://developer.mozilla.org/en-US/docs/HTTP/Access_control_CORS
                if(!const_cast<SCWsTransport*>(m_pcTransport)->sendData(oPeer->getFd(), kHttpOptionsResponse, kHttpOptionsResponseSize)) {
                    SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Failed to send response to HTTP OPTIONS");
                }
            }
            /* POST */
            else if(tsk_striequals(httpMessage->line.request.method, kHttpMethodPost)) {
                if(!isJsonContentType(httpMessage)) {
                    m_pcTransport->sendResult(oPeer, new SCWsResult(kSCWsResultCode_InvalidData, "Invalid content-type"));
                }
                else if(!THTTP_MESSAGE_HAS_CONTENT(httpMessage)) {
                    m_pcTransport->sendResult(oPeer, new SCWsResult(kSCWsResultCode_InvalidData, "Bodiless POST request not allowed"));
                }
                else {
                    SCObjWrapper<SCWsResult*> oResult = m_pcTransport->handleJsonContent(oPeer, THTTP_MESSAGE_CONTENT(httpMessage), THTTP_MESSAGE_CONTENT_LENGTH(httpMessage));
                    m_pcTransport->sendResult(oPeer, oResult);
                }
            }
        }

        /* Parse next chunck */
        if((nDataSize - nConsumedBytes) >= kStreamChunckMinSize) {
            TSK_OBJECT_SAFE_FREE(httpMessage);
            goto parse_buffer;
        }
    }

parse_rawcontent:
    if(oPeer->isRawContent()) {
        if(m_pcTransport->m_oFileUploads.find(oPeer->getFd()) == m_pcTransport->m_oFileUploads.end()) {
            SC_DEBUG_ERROR_EX(kSCMobuleNameWsTransport, "Failed to find local file");
            m_pcTransport->sendResult(oPeer, new SCWsResult(kSCWsResultCode_InternalError, "Failed to find local file"));
            nConsumedBytes = nDataSize;
            goto bail;
        }
        SCObjWrapper<SCNetFileUpload*> oFileUpload = const_cast<SCWsTransport*>(m_pcTransport)->m_oFileUploads[oPeer->getFd()];

        if(httpMessage) {
            const size_t nMessageContentLength = THTTP_MESSAGE_CONTENT_LENGTH(httpMessage);
            if(nMessageContentLength > 0 && !oFileUpload->getFileSize()) {
                oFileUpload->setFileSize(nMessageContentLength);
            }
        }

        nConsumedBytes += endOfheaders;
        if(endOfheaders < (int32_t)nDataSize) {
            const void* _pcDataPtr = pcDataPtr + endOfheaders;
            const size_t _nDataSize = nDataSize - endOfheaders;
            nConsumedBytes += oFileUpload->writeData(_pcDataPtr, _nDataSize);
        }

        if(oFileUpload->isAllDataWritten()) {
            SC_DEBUG_INFO_EX(kSCMobuleNameWsTransport, "We got the complete file :)");
            oFileUpload->close(); // close the file to be able to use it

            short nCode = kSCWsResultCode_Success;
            const char* pcPhrase = "We got the file";

            SCObjWrapper<OTBridge*>oBridge = OTEngine::getBridge(const_cast<SCWsTransport*>(m_pcTransport)->getEngineId(), oFileUpload->getBridgeId());
            if(oBridge) {
                SCObjWrapper<OTSipSessionAV*> oAVCall = oBridge->findCallByUserId(oFileUpload->getUserId());
                if(oAVCall) {
                    if(!oAVCall->presentationShare(oFileUpload->getFilePath())) {
                        nCode = kSCWsResultCode_InternalError;
                        pcPhrase = "Failed to share presentation";
                    }
                }
                else {
                    nCode = kSCWsResultCode_InternalError;
                    pcPhrase = "Failed to find call by user id";
                }
            }
            else {
                nCode = kSCWsResultCode_InternalError;
                pcPhrase = "Failed to find bridge";
            }
            m_pcTransport->sendResult(oPeer, new SCWsResult(nCode, pcPhrase)); // close the connection

            // start sharing
        }
    }

bail:
    TSK_OBJECT_SAFE_FREE(httpMessage);

    return true;
#endif
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

static SC_INLINE bool isJsonContentType(const thttp_message_t *pcHttpMessage)
{
    return isContentType(pcHttpMessage, kJsonContentType);
}
